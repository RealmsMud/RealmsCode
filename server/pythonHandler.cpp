/*
 * pythonHandler.h
 *   Class for handling Python <--> mud interactions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___ 
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <cstdlib>                                            // for getenv
#include <new>                                                 // for operat...
#include <iostream>
#include <ostream>                                             // for operat...
#include <string>                                              // for string

#include "commands.hpp"                                        // for doCast...
#include "config.hpp"                                          // for Config
#include "creatures.hpp"                                       // for Creature
#include "exits.hpp"                                           // for Exit
#include "fishing.hpp"                                         // for Fishing
#include "mudObject.hpp"                                       // for MudObject
#include "objects.hpp"                                         // for Object
#include "paths.hpp"                                           // for Python
#include "proto.hpp"                                           // for broadcast
#include "pythonHandler.hpp"                                   // for Python...
#include "rooms.hpp"                                           // for BaseRoom
#include "server.hpp"                                          // for Server
#include "socials.hpp"
#include "socket.hpp"                                          // for Socket

/*
 * We're looking to do two things here
 * 1) Embed python, so that we can call python functions from c++
 * 2) Make c++ functions available from the embeded python
 */

#define REALMS_MODULE(name)                                                         \
    void PYBIND11_CONCAT(init_module_, name)(py::module &m );                       \
    PYBIND11_EMBEDDED_MODULE(name, PYBIND11_CONCAT(name, Module) )  {               \
       PYBIND11_CONCAT(init_module_, name)(PYBIND11_CONCAT(name, Module));          \
    }

REALMS_MODULE(mud);
REALMS_MODULE(effects);
REALMS_MODULE(stats);
REALMS_MODULE(mudObject);


bool PythonHandler::initPython() {
    try {
        // Add in our python lib to the python path for importing modules
        setenv("PYTHONPATH", Path::Python, 1);
        std::clog << " ====> PythonPath: " << getenv("PYTHONPATH") << std::endl;

        gServer->pythonHandler = new PythonHandler();
        py::initialize_interpreter();

        py::module main = py::module::import("__main__");
        gServer->pythonHandler->mainNamespace = main.attr("__dict__");

        // Import the mud module
        auto mudModule = py::module::import("mud");
        main.attr("mud") = mudModule;
        // Make the gConfig & gServer singletons available
        mudModule.attr("gConfig") = gConfig;
        mudModule.attr("gServer") = gServer;

        // Import a few other modules
        auto sysModule = py::module::import("sys");
        main.attr("sys") = sysModule;

        auto mudObjectLibModule = py::module::import("mudObject");
        main.attr("mudObject") = mudObjectLibModule;

        auto mudLibModule = py::module::import("mudLib");
        main.attr("mudLib") = mudLibModule;

        // Say hello
        py::exec(R"(
            print(f"Pybind11 Initialized.  Running mud version {mud.gConfig.getVersion()}")
        )", gServer->pythonHandler->mainNamespace);

    }
    catch (py::error_already_set &e) {
        PyErr_Print();
        std::clog << "Error initializing Python: " << e.what() << std::endl;
        return false;
    }
    return true;
}


void PythonHandler::cleanUpPython() {
    // Delete first, then finalize the interpreter
    delete gServer->pythonHandler;
    py::finalize_interpreter();
}

bool PythonHandler::addMudObjectToDictionary(py::object& dictionary, const std::string& key, MudObject* myObject) {
    // If null, we still want it!
    if (!myObject) {
        dictionary[key.c_str()] = myObject;
        return true;
    }

    Monster* mPtr = myObject->getAsMonster();
    Player* pPtr = myObject->getAsPlayer();
    Object* oPtr = myObject->getAsObject();
    UniqueRoom* rPtr = myObject->getAsUniqueRoom();
    AreaRoom* aPtr = myObject->getAsAreaRoom();
    Exit* xPtr = myObject->getAsExit();

    if (mPtr) {
        dictionary[key.c_str()] = py::cast(mPtr);
    } else if (pPtr) {
        dictionary[key.c_str()] = py::cast(pPtr);
    } else if (oPtr) {
        dictionary[key.c_str()] = py::cast(oPtr);
    } else if (rPtr) {
        dictionary[key.c_str()] = py::cast(rPtr);
    } else if (aPtr) {
        dictionary[key.c_str()] = py::cast(aPtr);
    } else if (xPtr) {
        dictionary[key.c_str()] = py::cast(xPtr);
    } else {
        dictionary[key.c_str()] = py::cast(myObject);
    }
    return true;
}

bool PythonHandler::runPython(const std::string& pyScript, py::object& locals) {
    try {
        // Say hello
        py::exec(pyScript, gServer->pythonHandler->mainNamespace, locals);

    }  catch (py::error_already_set &e) {
        handlePythonError(e);
        return false;
    }
    return true;
}

//==============================================================================
// RunPython:
//==============================================================================
//  pyScript:   The script to be run
//  actor:      The actor of the script.
//  target:     The target of the script

bool PythonHandler::runPython(const std::string& pyScript, const std::string &args, MudObject *actor, MudObject *target) {
    auto locals = py::dict("args"_a=args);

    addMudObjectToDictionary(locals, "actor", actor);
    addMudObjectToDictionary(locals, "target", target);

    return (runPython(pyScript, locals));
}

bool PythonHandler::runPythonWithReturn(const std::string& pyScript, py::object& locals) {
    try {
        // Note: Using eval here without specifying py::eval_statements; so it'll need to be one line
        // Additionally, we're expecting to call a function that returns a bool
        return(py::eval(pyScript, gServer->pythonHandler->mainNamespace, locals).cast<bool>());
    }  catch (py::error_already_set &e) {
        handlePythonError(e);
        return false;
    }
}

//==============================================================================
// RunPythonWithReturn:
//==============================================================================
//  pyScript:   The script to be run
//  actor:      The actor of the script.
//  target:     The target of the script
bool PythonHandler::runPythonWithReturn(const std::string& pyScript, const std::string &args, MudObject *actor, MudObject *target) {

    try {
        auto locals = py::dict("args"_a=args);

        addMudObjectToDictionary(locals, "actor", actor);
        addMudObjectToDictionary(locals, "target", target);

        return runPythonWithReturn(pyScript, locals);
    }
    catch( py::error_already_set &e) {
        handlePythonError(e);
    }

    return(true);
}

void PythonHandler::handlePythonError(py::error_already_set &e) {
    auto what = e.what();
    std::clog << "Python Error: " << what << std::endl;
    broadcast(isDm, "^GPython Error: %s", what);

//    auto &ty=e.type(), &val=e.value(), &tb=e.value();
////    PyErr_NormalizeException(&exc,&val,&tb);
//    py::object traceback(py::module::import("traceback"));
//    py::object format_exception(traceback.attr("format_exception"));
//    py::object formatted_list(format_exception(ty, val, tb));
//    std::clog << formatted_list.cast<std::string>() << std::endl;

}

// Server Python Proxy Methods
bool Server::runPython(const std::string& pyScript, py::object& dictionary) {
    return(pythonHandler->runPython(pyScript, dictionary));
}
bool Server::runPythonWithReturn(const std::string& pyScript, py::object& dictionary) {
    return(pythonHandler->runPythonWithReturn(pyScript, dictionary));
}
bool Server::runPython(const std::string& pyScript, const std::string &args, MudObject *actor, MudObject *target) {
    return pythonHandler->runPython(pyScript, args, actor, target);
}
bool Server::runPythonWithReturn(const std::string& pyScript, const std::string &args, MudObject *actor, MudObject *target) {
    return pythonHandler->runPythonWithReturn(pyScript, args, actor, target);
}

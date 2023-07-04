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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#pragma once

// C++ Includes
#include <Python.h>

#include <pybind11/pytypes.h>
#include <pybind11/embed.h>

namespace py = pybind11;
using namespace py::literals;

class PythonHandler {
    friend class Server;
private:
    // Our main namespace for python
    py::object mainNamespace;

public:
    // Python
    static bool initPython();
    static void cleanUpPython();

    bool runPython(const std::string& pyScript, py::object& locals);
    bool runPythonWithReturn(const std::string& pyScript, py::object& locals);
    bool runPython(const std::string& pyScript, const std::string &args = "", std::shared_ptr<MudObject>actor = nullptr, std::shared_ptr<MudObject>target = nullptr);
    bool runPythonWithReturn(const std::string& pyScript, const std::string &args = "", std::shared_ptr<MudObject>actor = nullptr, std::shared_ptr<MudObject>target = nullptr);
    static void handlePythonError(py::error_already_set &e);

    static bool addMudObjectToDictionary(py::object& dictionary, const std::string& key, std::shared_ptr<MudObject> myObject);


};


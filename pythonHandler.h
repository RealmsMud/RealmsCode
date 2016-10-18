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
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */



#ifndef _PYTHONHANDLER_H
#define _PYTHONHANDLER_H

#ifndef PYTHON_CODE_GEN 
// C++ Includes
#include <Python.h>
#include <boost/python.hpp>


// Use the boost::python namespace
using namespace boost::python;


bool addMudObjectToDictionary(object& dictionary, bstring key, MudObject* myObject);

class PythonHandler {
    friend class Server;
private:
    // Our main namespace for python
    object mainNamespace;
    
};

#else

class PythonHandler;
class object;

#endif // PYTHON_CODE_GEN

#endif  /* _PYTHONHANDLER_H */


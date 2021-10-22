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



#ifndef _PYTHONHANDLER_H
#define _PYTHONHANDLER_H

// C++ Includes
#include <Python.h>

#include <pybind11/embed.h>

namespace py = pybind11;

class PythonHandler {
    friend class Server;
private:
    // Our main namespace for python
    py::object mainNamespace;

};

#endif  /* _PYTHONHANDLER_H */


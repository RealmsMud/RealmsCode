/*
 * help.cpp
 *   Routines related to help files
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

#include "mud.h"
#include <fstream>

namespace Help {
//**********************************************************************
//                      loadHelpTemplate
//**********************************************************************

bstring loadHelpTemplate(const char* filename) {
    char    file[80], line[200];
    bstring str;

    sprintf(file, "%s%s.txt", Path::HelpTemplate, filename);
    std::ifstream in(file);

    // if there isn't a template
    if(!in)
        return("");

    // read all the information from the template in
    while(!in.eof()) {
        in.getline(line, 200, '\n');
        str += line;
        str += "\n";
    }
    in.close();

    return(str);
}

}

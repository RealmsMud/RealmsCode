/*
 * mxp.cpp
 *   Code that deals with mxp
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

#ifndef MXP_H
#define MXP_H

#include <libxml/parser.h>  // for xmlNodePtr

class MxpElement {
public:
    MxpElement(xmlNodePtr rootNode);
    std::string getDefineString();

    std::string getName();
    std::string getColor();
    bool isColor();

protected:
    std::string name;
    std::string mxpType;
    std::string command;
    std::string hint;
    bool prompt;
    std::string attributes;
    std::string expire;
    std::string color;
};


#endif// MXP_H

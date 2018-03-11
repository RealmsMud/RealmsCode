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
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef MXP_H
#define MXP_H

#include "common.hpp"

class MxpElement {
public:
    MxpElement(xmlNodePtr rootNode);
    bstring getDefineString();

    bstring getName();
    bstring getColor();
    bool isColor();

protected:
    bstring name;
    bstring mxpType;
    bstring command;
    bstring hint;
    bool prompt;
    bstring attributes;
    bstring expire;
    bstring color;
};


#endif// MXP_H

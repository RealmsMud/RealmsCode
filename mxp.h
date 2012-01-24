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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef MXP_H
#define MXP_H


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

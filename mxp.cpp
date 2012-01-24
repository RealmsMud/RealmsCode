/*
 * mxp.cpp
 *   Functions that deal with mxp
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

#include "mud.h"
#include "mxp.h"

bstring MxpElement::getName() {
    return(name);
}
bstring MxpElement::getColor() {
    return(color);
}
bool MxpElement::isColor() {
    return(!color.empty());
}

//***********************************************************************
//                      defineMXP
//***********************************************************************
// TODO: Put this in a lookup table
void Socket::defineMxp() {
    if(!this || !getMxp())
        return;
//    bprint(MXP_SECURE_OPEN);
    bprint(MXP_BEG "VERSION" MXP_END);
//    bprint(MXP_BEG "!ELEMENT c1 '<COLOR #FFD700>'" MXP_END);    // gold
//    bprint(MXP_BEG "!ELEMENT c2 '<COLOR #009CFF>'" MXP_END);    // cerulean
//    bprint(MXP_BEG "!ELEMENT c3 '<COLOR #FF5ADE>'" MXP_END);    // pink
//    bprint(MXP_BEG "!ELEMENT c4 '<COLOR #82E6FF>'" MXP_END);    // sky blue
//    bprint(MXP_BEG "!ELEMENT c5 '<COLOR #484848>'" MXP_END);    // dark grey
//    bprint(MXP_BEG "!ELEMENT c6 '<COLOR #95601A>'" MXP_END);    // brown
//    bprint(MXP_LOCK_CLOSE);
    for(MxpElementMap::value_type p : gConfig->mxpElements) {
        bprint(p.second->getDefineString());
        std::cout << p.second->getDefineString() << std::endl;
    }

}


void Config::clearMxpElements() {
    for(MxpElementMap::value_type p : mxpElements) {
        delete p.second;
    }
    mxpElements.clear();
    mxpColors.clear();
}

bstring& Config::getMxpColorTag(bstring str) {
    return(mxpColors[str]);
}
//bstring name;
//MxpType mxpType;
//bstring command;
//bstring hint;
//bool prompt;
//bstring attributes;
//bstring expire;

bstring MxpElement::getDefineString() {
    std::ostringstream oStr;

    oStr << MXP_BEG;
    oStr << "!ELEMENT " << name;
    oStr << " \"<";
    if(mxpType == "send")
        oStr << "send href='";
    oStr << command;
    if(mxpType == "send")
        oStr << "' ";
    if(!hint.empty())
        oStr << "hint='" << hint << "' ";
    if(prompt)
        oStr << "prompt ";
    if(!expire.empty())
        oStr << "expire='" << expire << "' ";
    oStr << ">\"";
    if(!attributes.empty())
        oStr << " ATT='" << attributes << "' ";
    oStr << MXP_END;

    return(oStr.str());
}

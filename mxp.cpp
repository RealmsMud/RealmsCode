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
 *  GNU Affero General Public License v3 or later
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
    bprint(MXP_BEG "VERSION" MXP_END);
    bprint(MXP_BEG "SUPPORT" MXP_END);
    for(MxpElementMap::value_type p : gConfig->mxpElements) {
        std::cout << p.second->getDefineString() << std::endl;
        bprint(p.second->getDefineString());
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

bstring MxpElement::getDefineString() {
    std::ostringstream oStr;

    oStr << MXP_BEG;
    oStr << "!ELEMENT " << name;
    oStr << " \"<";
    if(mxpType == "send")
        oStr << "send href='";
    oStr << command;
    if(mxpType == "send")
        oStr << "'";
    if(!hint.empty())
        oStr << " hint='" << hint << "'";
    if(!expire.empty())
        oStr << " expire='" << expire << "'";
    if(prompt)
        oStr << " PROMPT";
    oStr << ">\"";
    if(!attributes.empty())
        oStr << " ATT='" << attributes << "'";
    oStr << MXP_END;

    return(oStr.str());
}

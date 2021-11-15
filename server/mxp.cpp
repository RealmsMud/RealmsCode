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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <map>         // for operator==, _Rb_tree_iterator, map, _Rb_tree_i...
#include <sstream>     // for operator<<, basic_ostream, char_traits, ostrin...
#include <string>      // for operator<<, string, operator==, basic_string
#include "config.hpp"  // for Config, MxpElementMap, stringMap, gConfig

#include "mxp.hpp"     // for MxpElement
#include "socket.hpp"  // for Socket, MXP_BEG, MXP_END

const std::string & MxpElement::getName() const {
    return(name);
}
const std::string & MxpElement::getColor() const {
    return(color);
}
bool MxpElement::isColor() const {
    return(!color.empty());
}

//***********************************************************************
//                      defineMXP
//***********************************************************************
// TODO: Put this in a lookup table
void Socket::defineMxp() {
    if(!mxpEnabled()) return;

    bprint(MXP_BEG "VERSION" MXP_END);
    bprint(MXP_BEG "SUPPORT" MXP_END);
    for(const auto& [mxpName, mxpElement] : gConfig->mxpElements) {
        bprint(mxpElement.getDefineString());
    }

}


void Config::clearMxpElements() {
    mxpElements.clear();
    mxpColors.clear();
}

std::string& Config::getMxpColorTag(const std::string &str) {
    return(mxpColors[str]);
}

std::string MxpElement::getDefineString() const {
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

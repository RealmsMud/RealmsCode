/*
 * prompt.cpp
 *   Player prompt routines.
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


#include <arpa/telnet.h>               // for IAC, EOR, GA
#include <ostream>                     // for operator<<, basic_ostream, ost...
#include <string>                      // for string, allocator, char_traits

#include "flags.hpp"                   // for P_DM_INVIS, P_CHAOTIC, O_DARKNESS
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/monsters.hpp"     // for Monster
#include "socket.hpp"                  // for Socket

//***********************************************************************
//                      sendPrompt
//***********************************************************************
// This function returns the prompt that the player should be seeing

void Player::sendPrompt() {
    auto sock = mySock.lock();
    if(!sock) return;

    std::string toPrint;

    if(fd < 0)
        return;

    // no prompt in this situation
    if(flagIsSet(P_SPYING) || flagIsSet(P_READING_FILE))
        return;

    if(flagIsSet(P_PROMPT) || flagIsSet(P_ALIASING)) {
        std::ostringstream promptStr;

        if(!flagIsSet(P_NO_EXTRA_COLOR))
            promptStr << alignColor();

        if(flagIsSet(P_ALIASING) && alias_crt) {
            promptStr << "(" << alias_crt->hp.getCur() << " H " << alias_crt->mp.getCur() << " M): ";
        } else {
            promptStr << "(" << hp.getCur() << " H";
            if(cClass != CreatureClass::LICH && cClass != CreatureClass::BERSERKER
               && (cClass != CreatureClass::FIGHTER || !flagIsSet(P_PTESTER)))
                promptStr << " " << mp.getCur() << " M";
            else if(cClass == CreatureClass::FIGHTER && flagIsSet(P_PTESTER))
                promptStr << " " << focus.getCur() << " F";

            if(flagIsSet(P_SHOW_XP_IN_PROMPT))
                promptStr << " " << expToLevel(true);
            promptStr << "):^x ";

        }
        toPrint = promptStr.str();
    } else
        toPrint = ": ";

    if(flagIsSet(P_AFK))
        toPrint += "^r[AFK]^x ";
    if(flagIsSet(P_NEWLINE_AFTER_PROMPT))
        toPrint += "\n";

    // Send EOR if they want it, otherwise send GA
    if(getSock()->eorEnabled()) {
        char eor_str[] = {(char)IAC, (char)EOR, '\0' };
        toPrint.append(eor_str);
    } else if(!getSock()->isDumbClient()){
        char ga_str[] = {(char)IAC, (char)GA, '\0' };
        toPrint.append(ga_str);
    }
    sock->write(toPrint);
}
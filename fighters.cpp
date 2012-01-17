/*
 * Fighters.cpp
 *   Code specific to fighters
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


//*********************************************************************
//                      increaseFocus
//*********************************************************************

void Player::increaseFocus(FocusAction action, int amt, Creature* target) {
    int focusIncrease = 0;

    // Only pure fighters have increased battle focus
    if(cClass != FIGHTER || cClass2 || !flagIsSet(P_PTESTER))
        return;

//    enum FocusAction {
//        FOCUS_DAMAGE_IN,
//        FOCUS_DAMAGE_OUT,
//        FOCUS_BLOCK,
//        FOCUS_PARRY,
//        FOCUS_CIRCLE,
//        FOCUS_BASH,
//        FOCUS_SPECIAL,
//
//        LAST_FOCUS
//    };

    // FactorIn = Average Rage Required per Ability / Number of hits for ability

    double factorIn = 5.0;

    double factorOut = 8.0;
    double a = 0.0;
    double c = 0.0;
    int levelDiff = 0;
    double lF = 0.0;
    double mod = 1.0;

    if(target) {
        levelDiff = tMAX(getLevel() - target->getLevel(), 0);
        lF = 1.0/tMAX(levelDiff/5.0,1.0);
    }
    switch(action) {
        case FOCUS_DAMAGE_IN:
            a = (-0.195130133 + (getLevel()*-0.128540388) + (hp.getMax() * 0.189543088) );
            c = factorIn / a;
            focusIncrease = amt * c;
            break;
        case FOCUS_BASH:
        case FOCUS_RIPOSTE:
            mod = 0.5;
            // Fall through to focus_damage_out calculation
        case FOCUS_DAMAGE_OUT:
            a = (-0.195130133 + (getLevel()*-0.128540388) + (hp.getMax() * 0.189543088) );
            c = factorOut / a;
            std::cout << "a: " << a << " c: " << c << " factor: " << factorOut << " lF: " << lF << std::endl;

            focusIncrease = amt * c * lF * mod;

            break;
        case FOCUS_DODGE:
        case FOCUS_BLOCK:
        case FOCUS_PARRY:
        case FOCUS_CIRCLE:
            focusIncrease = 2;
            break;
        case FOCUS_SPECIAL:
        default:
            break;
    }

    focusIncrease = MAX(1, focusIncrease);

    printColor("^rYour battle focus increases. (%d)\n", focusIncrease);

    focus.increase(focusIncrease);
}

// Slowly tick focus down
void Player::decreaseFocus() {
    // Only pure fighters have increased battle focus
    if(cClass != FIGHTER || cClass2 || !flagIsSet(P_PTESTER))
        return;

    if(focus.getCur() <= 0)
        return;

    int amt = 2;
    amt = MAX(amt, 1);

    // Modify amt here

    focus.decrease(amt);

    // Display focus reduction every 10%
    if(focus.getCur()%10 == 0)
        printColor("^CYour battle focus diminishes.\n");

}

// Clear the player's focus
void Player::clearFocus() {
    // Only pure fighters have increased battle focus
    if(cClass != FIGHTER || cClass2 || !flagIsSet(P_PTESTER))
        return;

    focus.setCur(0);
}

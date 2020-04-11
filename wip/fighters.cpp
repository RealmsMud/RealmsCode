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
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "creatures.hpp"  // for Player, Creature
#include "fighters.hpp"   // for FOCUS_BASH, FOCUS_BLOCK, FOCUS_CIRCLE, FOCU...
#include "flags.hpp"      // for P_PTESTER
#include "global.hpp"     // for CreatureClass, CreatureClass::FIGHTER
#include "utils.hpp"      // for MAX

// Used for new fighters, returns true if they're a pure fighter
// (and for now a ptester)
bool Player::isPureFighter() {
    return(cClass == CreatureClass::FIGHTER && !hasSecondClass() && flagIsSet(P_PTESTER));
}

//*********************************************************************
//                      increaseFocus
//*********************************************************************

void Player::increaseFocus(FocusAction action, int amt, Creature* target) {
    int focusIncrease = 0;

    // Only pure fighters have increased battle focus
    if(!isPureFighter())
        return;

    // Only tick if we're not unfocused
    if(isEffected("unfocused"))
        return;

    // FactorIn/out = Average Rage Required per Ability / Number of hits to generate that amount
    double factorIn = 5.0;
    double factorOut = 8.0;
    double a = 0.0;
    double c = 0.0;
    int levelDiff = 0;
    double lF = 0.0;
    double mod = 1.0;

    if(target) {
        // Focus earned on damage out is reduced by a scaling factor if the player
        // is higher level than the target
        levelDiff = MAX(getLevel() - target->getLevel(), 0);
        lF = 1.0/MAX(levelDiff/5.0,1.0);
    }
    switch(action) {
        case FOCUS_DAMAGE_IN:
            // Approximation of average damage for monsters at a given level
            // based on the player's level and maximum hitpoints

            a = (-0.195130133 + (getLevel()*-0.128540388) + (hp.getMax() * 0.189543088) );
            c = factorIn / a;
            focusIncrease = amt * c;
            break;
        case FOCUS_BASH:
        case FOCUS_RIPOSTE:
            mod = 0.5;
            // no break - Fall through to focus damage out calculation with a different mod amount
        case FOCUS_DAMAGE_OUT:
            // Approximation of average damage for monsters at a given level
            // based on the player's level and maximum hitpoints
            a = (-0.195130133 + (getLevel()*-0.128540388) + (hp.getMax() * 0.189543088) );
            c = factorOut / a;

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

    focusIncrease = MAX(0, focusIncrease);
    if(focusIncrease) {
        printColor("^rYour battle focus increases. (%d)\n", focusIncrease);

        focus.increase(focusIncrease);
    }
}

// Slowly tick focus down
void Player::decreaseFocus() {
    // Only pure fighters have increased battle focus
    if(!isPureFighter())
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
    if(!isPureFighter())
        return;

    focus.setCur(0);
}

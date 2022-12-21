/*
 * playerEffects.cpp
 *   Player Effect related routines.
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

#include "flags.hpp"                        // for P_DM_INVIS, P_CHAOTIC, O_DARKNESS
#include "mud.hpp"                          // for LT, LT_PLAYER_SEND, LT_AGE
#include "mudObjects/players.hpp"           // for Player
#include "mudObjects/uniqueRooms.hpp"       // for UniqueRoom
#include "proto.hpp"                        // for bonus, broadcast, abortFindRoom

//*********************************************************************
//                      checkEffectsWearingOff
//*********************************************************************

void Player::checkEffectsWearingOff() {
    long t = time(nullptr);
    int staff = isStaff();

    // Added P_STUNNED and LT_PLAYER_STUNNED stun for dodge code.
    if(flagIsSet(P_STUNNED)) {
        if(t > LT(this, LT_PLAYER_STUNNED)) {
            clearFlag(P_STUNNED);
        }
    }

    if(flagIsSet(P_FOCUSED) && (cClass == CreatureClass::MONK || staff)) {
        if(t > LT(this, LT_FOCUS)) {
            printColor("^cYou lose your concentration.\n");
            clearFlag(P_FOCUSED);
            //computeAC();
            computeAttackPower();
        }
    }
    if(flagIsSet(P_MISTBANE)) {
        if(t > LT(this, LT_FOCUS)) {
            printColor("^bYour mistbane is ended.\n");
            clearFlag(P_MISTBANE);
        }
    }

    if(flagIsSet(P_OUTLAW)) {
        if(t > LT(this, LT_OUTLAW)) {
            printColor("^yYou are no longer an outlaw.\n");
            clearFlag(P_OUTLAW);
            clearFlag(P_OUTLAW_WILL_BE_ATTACKED);
            setFlag(P_NO_SUMMON);
            clearFlag(P_OUTLAW_WILL_LOSE_XP);
            clearFlag(P_NO_GET_ALL);
        }
    }

    // only force them to wake from sleeping unconsciousness when they're
    // completely healed.
    if(flagIsSet(P_UNCONSCIOUS)) {
        if( (   !flagIsSet(P_SLEEPING) && t > LT(this, LT_UNCONSCIOUS)) ||
            (   flagIsSet(P_SLEEPING) && ((hp.getCur() >= hp.getMax() && mp.getCur() >= mp.getMax()) || (cClass == CreatureClass::PUREBLOOD && !getRoomParent()->vampCanSleep(getSock()))) ))
        {
            printColor("^cYou wake up.\n");
            clearFlag(P_UNCONSCIOUS);
            wake();

            clearFlag(P_DIED_IN_DUEL);
            broadcast(getSock(), getRoomParent(), "%M wakes up.", this);
        }
    }

    if(flagIsSet(P_NO_SUICIDE)) {
        if(t > LT(this, LT_MOBDEATH)) {
            printColor("^yYour cooling-off period has ended.\n");
            clearFlag(P_NO_SUICIDE);
        }
    }
    if(flagIsSet(P_HIDDEN) && !staff) {
        if(t - lasttime[LT_HIDE].ltime > 300L) {
            printColor("^cShifting shadows expose you.\n");
            unhide(false);
        }
    }
    if(flagIsSet(P_FREE_ACTION)) {
        if(t > LT(this, LT_FREE_ACTION)) {
            printColor("^c^#You no longer magically move freely.\n");
            clearFlag(P_FREE_ACTION);
            computeAC();
            computeAttackPower();
        }
    }

    if(flagIsSet(P_NO_PKILL)) {
        if(t > LT(this, LT_NO_PKILL)) {
            printColor("^c^#You can now be pkilled again.\n");
            clearFlag(P_NO_PKILL);
        }
    }



    if(flagIsSet(P_DOCTOR_KILLER)) {
        if(t > LT(this, LT_KILL_DOCTOR) || staff) {
            printColor("^y^#The doctors have forgiven you.\n");
            clearFlag(P_DOCTOR_KILLER);
        }
    }


    if(flagIsSet(P_NO_TICK_MP)) {
        if(t > LT(this, LT_NOMPTICK) || staff) {
            printColor("^cYour magical vitality has returned.\n");
            clearFlag(P_NO_TICK_MP);
        }

    }

    if(flagIsSet(P_NO_TICK_HP)) {
        if(t > LT(this, LT_NOHPTICK) || staff) {
            printColor("^gYou now heal normally again.\n");
            clearFlag(P_NO_TICK_HP);
        }
    }

    if(flagIsSet(P_CANT_BROADCAST)) {
        if(t > LT(this, LT_NO_BROADCAST)) {
            printColor("^rYou can broadcast again, now don't abuse it this time.\n");
            clearFlag(P_CANT_BROADCAST);
        }
    }

    if(isEffected("mist")) {
        if(isDay() && !staff)
            unmist();
    }
    if(flagIsSet(P_CHARMED)) {
        if(t > LT(this, LT_CHARMED) || staff) {
            printColor("^yYou are again in control of your actions.\n");
            clearFlag(P_CHARMED);
        }
    }

    if(negativeLevels) {
        if(t > LT(this, LT_LEVEL_DRAIN) || staff) {
            unsigned long expTemp=0;
            negativeLevels--;
            if(negativeLevels) {
                printColor("^WYou have regained a lost level.\n");
                expTemp = experience;
                upLevel();
                experience = expTemp;

                lasttime[LT_LEVEL_DRAIN].ltime = t;
                lasttime[LT_LEVEL_DRAIN].interval = 60L + 5*bonus(constitution.getCur());
            } else {
                printColor("^WYou have recovered all your lost levels.\n");
                expTemp = experience;
                upLevel();
                experience = expTemp;
            }
        }
    }

    if(t > LT(this, LT_JAILED) && flagIsSet(P_JAILED)) {
        printColor("^rA demonic jailer just arrived.\n");
        printColor("The demonic jailer says, \"You have been released from your torment.\"\n");
        printColor("The demonic jailer casts word of recall on you.\n");

        broadcast(getSock(), getRoomParent(), "A demonic jailer just arrived.\nThe demonic jailer casts word of recall on %s.", getCName());
        broadcast(getSock(), getRoomParent(), "The demonic jailer sneers evilly and spits on you.\nThe demonic jailer vanishes.");
        broadcast("^R### Cackling demons shove %s from the Dungeon of Despair.", getCName());
        doRecall();

        clearFlag(P_JAILED);
    }


    if( t > LT(this, LT_MOB_JAILED) && inUniqueRoom() && getUniqueRoomParent()->flagIsSet(R_MOB_JAIL) && !staff) {
        printColor("A jailer just arrived.\n");
        printColor("The jailer says, \"You're free to go...get out!\"\n");
        printColor("The jailer casts word-of-recall on you.\n");
        broadcast(getSock(), getRoomParent(), "The jailer says to %s, \"You're free to go. Get out!\"", getCName());
        broadcast(getSock(), getRoomParent(), "The jailer casts word-of-recall on %s.", getCName());
        broadcast(getSock(), getRoomParent(), "The jailer eyes you suspiciously and leaves.");
        doRecall();
    }
}
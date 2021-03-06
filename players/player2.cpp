/*
 * player2.cpp
 *   Player routines.
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

#include <cctype>          // for isspace
#include <cstdio>          // for sprintf
#include <cstdlib>         // for atoi
#include <cstring>         // for strtok, strcpy, strcat, strdup, strstr

#include "bstring.hpp"      // for bstring, operator+
#include "catRef.hpp"       // for CatRef
#include "catRefInfo.hpp"   // for CatRefInfo
#include "cmd.hpp"          // for cmd
#include "commands.hpp"     // for cmdAction
#include "config.hpp"       // for Config, gConfig
#include "creatures.hpp"    // for Player, Creature
#include "exits.hpp"        // for Exit
#include "flags.hpp"        // for M_UNKILLABLE
#include "global.hpp"       // for CreatureClass, CON, CreatureClass::LICH
#include "levelGain.hpp"    // for LevelGain
#include "monType.hpp"      // for INVALID, PLAYER, mType
#include "objects.hpp"      // for Object
#include "os.hpp"           // for merror
#include "playerClass.hpp"  // for PlayerClass
#include "proto.hpp"        // for broadcast, getRandomMonster, getRandomPlayer
#include "raceData.hpp"     // for RaceData
#include "random.hpp"       // for Random
#include "rooms.hpp"        // for BaseRoom, UniqueRoom (ptr only)
#include "structs.hpp"      // for vstat
#include "utils.hpp"        // for MAX, MIN
#include "xml.hpp"          // for loadRoom

//********************************************************************
//                      calcStats
//********************************************************************

void Player::calcStats(vstat sendStat, vstat *toStat) {
    int         i=0, levels = 0, switchNum=0;
    int         lvl=0, r=0;
    int         hptemp=0,mptemp=0;

    CreatureClass cls=CreatureClass::NONE, cls2=CreatureClass::NONE;
    // stats are 1 based, the array is 0 based, so give us extra room
    int     num[MAX_STAT+1];



    if(sendStat.race)
        r = tstat.race;
    else
        r = race;

    if(sendStat.cls != CreatureClass::NONE)
        cls = tstat.cls;
    else
        cls = cClass;

    if(sendStat.cls2 != CreatureClass::NONE)
        cls2 = tstat.cls2;
    else
        cls2 = cClass2;

    if(sendStat.level)
        lvl = tstat.level;
    else
        lvl = level;


    for(i=1; i<=MAX_STAT; i++) {
        num[i] = sendStat.num[i-1] * 10;
        num[i] += gConfig->getRace(r)->getStatAdj(i);
        num[i] = MAX(1, num[i]);
    }

    PlayerClass *pClass = gConfig->classes[getClassString()];
    //const RaceData* rData = gConfig->getRace(race);
    LevelGain *lGain = nullptr;

    // Check for level info
    if(!pClass) {
        print("Error: Can't find your class!\n");
        if(!isStaff()) {
            bstring errorStr = "Error: Can't find class: " + getClassString();
            merror(errorStr.c_str(), NONFATAL);
        }
        return;
    } 


    // Now to adjust hit points accordingly
    hptemp = pClass->getBaseHp();
    if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH)
        mptemp = pClass->getBaseMp();


    for(levels = 1; levels <= lvl; levels ++) {
        lGain = pClass->getLevelGain(levels);
        if(!lGain) {
            print("Error: Can't find any information for level %d!\n", levels);
            if(!isStaff()) {
                bstring errorStr = "Error: Can't find level info for " + getClassString() + level;
                merror(errorStr.c_str(), NONFATAL);
            }
            continue;
        }

        switchNum = lGain->getStat();
        num[switchNum] += 10;

        // Now to adjust hit points accordingly
        hptemp += lGain->getHp();

        if(cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH)
            mptemp += lGain->getMp();

        if(cls != CreatureClass::LICH) {
            if(cls == CreatureClass::BERSERKER && num[CON] >= 70)
                hptemp++;
            if(num[CON] >= 130)
                hptemp++;
            if(num[CON] >= 210)
                hptemp++;
            if(num[CON] >= 250)
                hptemp++;
        }

        // liches gain an extra HP at every even level.
        if(levels % 2 == 0 && cls == CreatureClass::LICH)
            hptemp++;
    }

    for(i=0; i<MAX_STAT; i++)
        toStat->num[i] = num[i+1];

    toStat->hp = hptemp;
    toStat->mp = mptemp;
}

//********************************************************************
//                      checkConfusion
//********************************************************************

bool Player::checkConfusion() {
    int     action=0, dmg=0;
    mType   targetType = PLAYER;
    char    atk[50];
    Creature* target=nullptr;
    Exit    *newExit=nullptr;
    BaseRoom* room = getRoomParent(), *bRoom=nullptr;
    CatRef  cr;


    if(!isEffected("confusion"))
        return(false);

    action = Random::get(1,4);
    switch(action) {
    case 1: // Stand confused

        broadcast(getSock(), room, "%M stands with a confused look on %s face.", this, hisHer());
        printColor("^BYou are confused and dizzy. You stand and look around cluelessly.\n");
        stun(Random::get(5,10));
        return(true);
        break;
    case 2: // Wander to random exit

        newExit = getFleeableExit();
        if(!newExit)
            return(false);
        bRoom = getFleeableRoom(newExit);
        if(!bRoom)
            return(false);

        printColor("^BWanderlust overtakes you.\n");
        printColor("^BYou wander aimlessly to the %s exit.\n", newExit->getCName());
        broadcast(getSock(), room, "%M wanders aimlessly to the %s exit.", this, newExit->getCName());
        deleteFromRoom();
        addToRoom(bRoom);
        doPetFollow();
        return(true);
        break;
    case 3: // Attack something randomly
        if(!checkAttackTimer(false))
            return(false);

        switch(Random::get(1,2)) {
        case 1:
            target = getRandomMonster(room);
            if(!target)
                target = getRandomPlayer(room);
            if(target) {
                targetType = target->getType();
                break;
            }
            break;
        case 2:
            target = getRandomPlayer(room);
            if(!target)
                target = getRandomMonster(room);
            if(target) {
                targetType = target->getType();
                break;
            }
            break;
        default:
            break;
        }

        if(!target || target == this)
            targetType = INVALID;


        switch(targetType) {
        case PLAYER: // Random player in room
            printColor("^BYou are convinced %s is trying to kill you!\n", target->getCName());
            attackCreature(target);
            return(true);
            break;
        case INVALID: // Self

            if(ready[WIELD-1]) {
                dmg = ready[WIELD-1]->damage.roll() +
                      bonus(strength.getCur()) + ready[WIELD - 1]->getAdjustment();
                printColor("^BYou frantically swing your weapon at imaginary enemies.\n");
            } else {
                printColor("^BYou madly flail around at imaginary enemies.\n");
                if(cClass == CreatureClass::MONK) {
                    dmg = Random::get(1,2) + level/3 + Random::get(1,(1+level)/2);
                    if(strength.getCur() < 90) {
                        dmg -= (90-strength.getCur())/10;
                        dmg = MAX(1,dmg);
                    }
                } else
                    dmg = damage.roll();
            }

            getDamageString(atk, this, ready[WIELD - 1]);

            printColor("^BYou %s yourself for %d damage!\n", atk, dmg);

            broadcast(getSock(), room, "%M %s %sself!", this, atk, himHer());
            hp.decrease(dmg);
            if(hp.getCur() < 1) {

                hp.setCur(1);

                printColor("^BYou accidentally killed yourself!\n");
                broadcast("### Sadly, %s accidentally killed %sself.", getCName(), himHer());

                mp.setCur(1);

                if(!inJail()) {
                    bRoom = getLimboRoom().loadRoom(this);
                    if(bRoom) {
                        deleteFromRoom();
                        addToRoom(bRoom);
                        doPetFollow();
                    }
                }
            }
            return(true);
            break;
        default: // Random monster in room.
            if(target->flagIsSet(M_UNKILLABLE))
                return(false);

            printColor("^BYou think %s is attacking you!\n", target);
            broadcast(getSock(), room, "%M yells, \"DIE %s!!!\"\n", this, target->getCName());
            attackCreature(target);
            return(true);
            break;
        }
        break;
    case 4: // flee
        printColor("^BPhantom dangers in your head beckon you to flee!\n");
        broadcast(getSock(), room, "%M screams in terror, pointing around at everything.", this);
        flee();
        return(true);
        break;
    default:
        break;
    }
    return(false);
}

//********************************************************************
//                      getEtherealTravelRoom
//********************************************************************

CatRef getEtherealTravelRoom() {
    const CatRefInfo* eth = gConfig->getCatRefInfo("et");
    CatRef cr;
    cr.setArea("et");
    cr.id = Random::get(1, eth->getTeleportWeight());
    return(cr);
}

//********************************************************************
//                      etherealTravel
//********************************************************************

void etherealTravel(Player* player) {
    UniqueRoom  *newRoom=nullptr;
    CatRef  cr = getEtherealTravelRoom();

    if(!loadRoom(cr, &newRoom))
        return;

    player->deleteFromRoom();
    player->addToRoom(newRoom);
    player->doPetFollow();
}

//********************************************************************
//                      cmdVisible
//********************************************************************

int cmdVisible(Player* player, cmd* cmnd) {
    if(!player->isInvisible()) {
        player->print("You are not invisible.\n");
        return(0);
    } else {
        player->removeEffect("invisibility");
        player->removeEffect("greater-invisibility");
    }
    return(0);
}

//********************************************************************
//                      cmdDice
//********************************************************************

int cmdDice(Creature* player, cmd* cmnd) {
    char    *str=nullptr, *tok=nullptr, diceOutput[256], add[256];
    int     strLen=0, i=0;
    int     diceSides=0,diceNum=0,diceAdd=0;
    int     rolls=0, total=0;

    const char *Syntax =    "\nSyntax: dice 1d2\n"
                    "        dice 1d2+3\n";

    strcpy(diceOutput, "");
    strcpy(add ,"");

    strLen = cmnd->fullstr.length();

    // This kills all leading whitespace
    while(i<strLen && isspace(cmnd->fullstr[i]))
        i++;
    // This kills the command itself
    while(i<strLen && !isspace(cmnd->fullstr[i]))
        i++;

    str = strstr(&cmnd->fullstr[i], "d");
    if(!str)
        return(cmdAction(player, cmnd));

    str = strdup(&cmnd->fullstr[i]);
    if(!str) {
        player->print(Syntax);
        return(0);
    }

    tok = strtok(str, "d");
    if(!tok) {
        player->print(Syntax);
        return(0);
    }
    diceNum = atoi(tok);

    tok = strtok(nullptr, "+");
    if(!tok) {
        player->print(Syntax);
        return(0);
    }
    diceSides = atoi(tok);

    tok = strtok(nullptr, "+");

    if(tok)
        diceAdd = atoi(tok);

    if(diceNum < 0) {
        player->print("How can you roll a negative number of dice?\n");
        return(0);
    }

    diceNum = MAX(1, diceNum);

    if(diceSides<2) {
        player->print("A die has a minimum of 2 sides.\n");
        return(0);
    }

    diceNum = MIN(100, diceNum);
    diceSides = MIN(100, diceSides);
    diceAdd = MAX(-100,MIN(100, diceAdd));


    sprintf(diceOutput, "%dd%d", diceNum, diceSides);
    if(diceAdd) {
        if(diceAdd > 0)
            sprintf(add, "+%d", diceAdd);
        else
            sprintf(add, "%d", diceAdd);
        strcat(diceOutput, add);
    }


    for(rolls=0;rolls<diceNum;rolls++)
        total += Random::get(1, diceSides);

    total += diceAdd;


    player->print("You roll %s\n: %d\n", diceOutput, total);
    broadcast(player->getSock(), player->getParent(), "(Dice %s): %M got %d.", diceOutput, player, total );

    return(0);
}


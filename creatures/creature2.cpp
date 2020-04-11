/*
 * creature2.cpp
 *   Additional creature routines.
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
#include <cstdlib>                // for abs
#include <ctime>                  // for time
#include <string>                 // for operator==, basic_string

#include "bstring.hpp"            // for bstring
#include "container.hpp"          // for PlayerSet, MonsterSet
#include "creatures.hpp"          // for Monster, Creature, Player
#include "flags.hpp"              // for M_FIRE_AURA, O_JUST_LOADED, M_CLASS...
#include "global.hpp"             // for BRE, MAXALVL, MAX_AURAS, CreatureClass
#include "monType.hpp"            // for getHitdice, HUMANOID
#include "money.hpp"              // for Money, GOLD
#include "mud.hpp"                // for LT_TICK, LT_TICK_SECONDARY, LT_M_AU...
#include "objects.hpp"            // for Object
#include "proto.hpp"              // for bonus, get_perm_ac, new_scroll, get...
#include "random.hpp"             // for Random
#include "rooms.hpp"              // for BaseRoom
#include "utils.hpp"              // for MAX, MIN
#include "xml.hpp"                // for loadObject

typedef struct {
    short   hpstart;
    short   mpstart;
    short   hp;
    short   mp;
    short   ndice;
    short   sdice;
    short   pdice;
} class_stats_struct;

class_stats_struct class_stats[static_cast<int>(CreatureClass::CLASS_COUNT)] = {
        {  0,  0,  0,  0,  0,  0,  0},
        { 19,  2,  6,  2,  1,  6,  0},  // assassin
        { 24,  0,  8,  1,  1,  3,  1},  // barbarian
        { 16,  4,  5,  4,  1,  4,  0},  // cleric
        { 22,  2,  7,  2,  1,  5,  0},  // fighter
        { 14,  5,  4,  5,  1,  3,  0},  // mage
        { 19,  3,  6,  3,  1,  4,  0},  // paladin
        { 18,  3,  6,  3,  2,  2,  0},  // ranger
        { 18,  3,  5,  2,  2,  2,  1},  // thief
        { 15,  3,  5,  4,  2,  2,  1},  // pureblood
        { 17,  3,  5,  2,  1,  3,  0},  // monk
        { 19,  3,  6,  3,  1,  4,  0},  // death-knight
        { 15,  4,  5,  4,  1,  3,  0},  // druid
        { 16,  0,  6,  0,  1,  3,  1},  // lich
        { 20,  1,  6,  2,  1,  3,  1},  // werewolf
        { 17,  3,  5,  4,  2,  2,  0},  // bard
        { 18,  3,  5,  2,  2,  2,  1},  // rogue
        { 30, 30, 10, 10,  5,  5,  5},  // builder
        { 30, 30, 10, 10,  5,  5,  5},  // unused
        { 30, 30, 10, 10,  5,  5,  5},  // caretaker
        { 30, 30, 10, 10,  5,  5,  5}   // dungeonmaster
};


typedef struct {
    unsigned short  armor;
    short   thaco;
    unsigned long   experience;
    unsigned short  ndice;
    unsigned short  sdice;
    short   pdice;
} balanceStat;
balanceStat balancedStats[41]  = {
    // armor, thaco, experience, ndice, sdice, pdice
    {0,0,0,0,0,0},              // 0
    {25,20,6,1,4,0},            // 1
    {50,19,15,2,3,0},           // 2
    {75,18,35,2,3,2},           // 3
    {100,17,50,2,3,2},          // 4
    {125,16,65,2,3,4},          // 5
    {150,15,75,4,2,4},          // 6
    {175,14,100,2,4,6},         // 7
    {200,13,130,4,3,4},         // 8
    {225,12,180,5,3,3},         // 9
    {250,11,240,3,5,5},         // 10
    {275,10,325,3,6,4},         // 11
    {300,9,400,3,6,6},          // 12
    {325,8,550,4,4,8},          // 13
    {350,7,700,4,5,8},          // 14
    {390,6,850,5,5,5},          // 15
    {430,5,1000,4,6,8},         // 16
    {470,4,1150,4,6,9},         // 17
    {510,3,1300,5,6,8},         // 18
    {550,2,1450,6,5,10},        // 19
    {590,1,1600,4,7,10},        // 20
    {630,0,1800,9,5,4},         // 21
    {670,-2,2000,8,5,10},       // 22
    {710,-4,2500,10,5,6},       // 23
    {750,-7,3000,10,5,8},       // 24
    {790,-8,3500,15,4,0},       // 25
    {830,-8,4000,20,3,4},       // 26
    {870,-8,5500,13,4,12},      // 27
    {920,-8,7000,13,5,5},       // 28
    {960,-9,8500,15,4,10},      // 29
    {1000,-10,10000,18,3,16},   // 30
    {1040,-10,11625,19,3,17},   // 31
    {1080,-10,12000,20,3,17},   // 32
    {1120,-10,12375,21,3,18},   // 33
    {1160,-11,12750,22,3,18},   // 34
    {1200,-12,13125,23,3,19},   // 35
    {1240,-12,13500,24,3,19},   // 36
    {1280,-12,13875,25,3,20},   // 37
    {1320,-12,14250,26,3,20},   // 38
    {1360,-13,14625,27,3,21},   // 39
    {1400,-14,15000,28,3,23}    // 40
};

void Monster::adjust(int buffswitch) {
    int     buff=0, crthp=0;
    long    xpmod=0;

    if(buffswitch == -1)
        buff = Random::get(1,3)-1;
    else
        buff = MAX(0, MIN(buffswitch, 2));


    /*          Web Editor
     *           _   _  ____ _______ ______
     *          | \ | |/ __ \__   __|  ____|
     *          |  \| | |  | | | |  | |__
     *          | . ` | |  | | | |  |  __|
     *          | |\  | |__| | | |  | |____
     *          |_| \_|\____/  |_|  |______|
     *
     *      If you change anything here, make sure the changes are reflected in the web
     *      editor! Either edit the PHP yourself or tell Dominus to make the changes.
     */
    if(!flagIsSet(M_CUSTOM)) {
        if(level >= 1 && level <= MAXALVL) {
            setArmor(balancedStats[level].armor);
            weaponSkill = (level-1)*10;
            defenseSkill = (level-1)*10;
            setExperience(balancedStats[level].experience);
            damage.setNumber(balancedStats[level].ndice);
            damage.setSides(balancedStats[level].sdice);
            damage.setPlus(balancedStats[level].pdice);
        } else {
            setArmor(100);
            weaponSkill = 1;
            defenseSkill = 1;
            setExperience(6);
            damage.setNumber(1);
            damage.setSides(4);
            damage.setPlus(0);
        }


        if(cClass == CreatureClass::NONE)
            hp.setInitial(level * monType::getHitdice(type));
        else {
            crthp = class_stats[(int) cClass].hpstart + (level*class_stats[(int) cClass].hp);
            hp.setInitial(MAX(crthp, (level * monType::getHitdice(type))));
        }

        hp.restore();

        xpmod = experience / 10;
        xpmod *= monType::getHitdice(type) - monType::getHitdice(HUMANOID);

        if(xpmod > 0)
            addExperience(xpmod);
        else
            subExperience(xpmod*-1);
    }


    switch(buff) {
    case 1:         // Mob is weak.
        hp.setInitial( hp.getMax() - hp.getMax()/5);
        hp.restore();
        coins.set(coins[GOLD] * 4 / 5, GOLD);

        subExperience(experience / 5);
        setArmor(armor - 100);
        damage.setNumber(damage.getNumber() * 9 / 10);
        damage.setPlus(damage.getPlus() * 9 / 10);
        break;
    case 2:         // Mob is stronger
        hp.setInitial( hp.getMax() + hp.getMax()/5);
        hp.restore();
        coins.set(coins[GOLD] * 6 / 5, GOLD);

        addExperience(experience / 5);
        defenseSkill += 5;
        weaponSkill += 5;
        setArmor(armor + 100);

        damage.setNumber(damage.getNumber() * 11 / 10);
        damage.setPlus(damage.getPlus() * 11 / 10);
        break;
    default:
        break;
    }

    armor = MAX<unsigned int>(MIN(armor, MAX_ARMOR), 0);

    if(level >= 7)
        setFlag(M_BLOCK_EXIT);

}

// *********************************************************
// initMonster
// *********************************************************
// Parameters:  <creature> The creature being initialized
//              <Opt:loadOriginal> Don't randomize align/gold
//              <Opt:prototype> Load all objs in inventory

// Initializes last times, inventory, scrolls, etc
int Monster::initMonster(bool loadOriginal, bool prototype) {
    int n=0, alnum=0, x=0;
    long t=0;
    Object* object=nullptr;

    t = time(nullptr);
    // init the timers
    lasttime[LT_MON_SCAVANGE].ltime =
    lasttime[LT_MON_WANDER].ltime =
    lasttime[LT_MOB_THIEF].ltime =
    lasttime[LT_TICK].ltime = t;
    lasttime[LT_TICK_SECONDARY].ltime = t;
    lasttime[LT_TICK_HARMFUL].ltime = t;

    // Make sure armor is set properly
    if(armor < (unsigned)(balancedStats[MIN<short>(level, MAXALVL)].armor - 150)) {
        armor = balancedStats[MIN<short>(level, MAXALVL)].armor;
    }

    if(dexterity.getCur() < 200)
        setAttackDelay(30);
    else
        setAttackDelay(20);

    if(flagIsSet(M_FAST_TICK))
        lasttime[LT_TICK].interval = lasttime[LT_TICK_SECONDARY].interval = 15L;
    else if(flagIsSet(M_REGENERATES))
        lasttime[LT_TICK].interval = lasttime[LT_TICK_SECONDARY].interval = 15L;
    else
        lasttime[LT_TICK].interval = lasttime[LT_TICK_SECONDARY].interval = 60L - (2*bonus(constitution.getCur()));

    lasttime[LT_TICK_HARMFUL].interval = 30;
    // Randomize alignment and gold unless we don' want it
    if(!loadOriginal) {
        if(flagIsSet(M_ALIGNMENT_VARIES) && alignment != 0) {
            alnum = Random::get(1,100);
            if(alnum == 1)
                alignment = 0;
            else if(alnum < 51)
                alignment = Random::get((short)1, (short)std::abs(alignment)) * -1;
            else
                alignment = Random::get((short)1, (short)std::abs(alignment));
        }

        if(!flagIsSet(M_NO_RANDOM_GOLD) && coins[GOLD])
            coins.set(Random::get(coins[GOLD]/10, coins[GOLD]), GOLD);
    }
    // Check for loading of random scrolls
    if(checkScrollDrop()) {
        n = new_scroll(level, &object);
        if(n > 0) {
            object->value.zero();
            addObj(object);
            object->setFlag(O_JUST_LOADED);
        }
    }

    // Now load up any always drop objects (Trading perms don't drop inventory items)
    if(!flagIsSet(M_TRADES)) {
        for(x=0;x<10;x++) {
            if(!loadObject(carry[x].info, &object))
                continue;
            object->init(!prototype);
            if( object->flagIsSet(O_ALWAYS_DROPPED) &&
                !object->getName().empty() &&
                object->getName()[0] != ' ' )
            {
                addObj(object);
                object->setFlag(O_JUST_LOADED);
            } else {
                delete object;
                continue;
            }
        }

        object = nullptr;

        int numDrops = Random::get(1,100), whichDrop=0;

             if(numDrops<90)    numDrops=1;
        else if(numDrops<96)    numDrops=2;
        else                    numDrops=3;

        if(prototype)
            numDrops = 10;
        for(x=0; x<numDrops; x++) {
            if(prototype)
                whichDrop=x;
            else
                whichDrop = Random::get(0,9);
            if(carry[whichDrop].info.id && !flagIsSet(M_TRADES)) {
                if(!loadObject(carry[whichDrop].info, &object))
                    continue;
                if( object->getName().empty() || object->getName()[0] == ' ')
                {
                    delete object;
                    continue;
                }

                object->init(!prototype);

                // so we don't get more than one always drop item.
                if(object->flagIsSet(O_ALWAYS_DROPPED)) {
                    delete object;
                    continue;
                }

                object->value.set(Random::get((object->value[GOLD]*9)/10,(object->value[GOLD]*11)/10), GOLD);

                addObj(object);
                object->setFlag(O_JUST_LOADED);
            }
        }
    }

    if(weaponSkill < (level * 3))
        weaponSkill = (level-1) * Random::get(9,11);
    return(1);
}


//*********************************************************************
//                      getMobNum
//*********************************************************************
// This function determines which one of the same kind of monster
// is in a room. For example, if there are three winos, it will
// return which number wino it is - 1, 2, or 3. - Tim C.

int Monster::getNumMobs() const {
    int     i=0;

    if(flagIsSet(M_DM_FOLLOW) || flagIsSet(M_WAS_PORTED))
        return(0);
    if(!this->inRoom())
        return(0);
    const BaseRoom* room = getConstRoomParent();
    if(!room)
        return(0);
    for(Monster* mons : room->monsters) {
        if(mons->getName() == getName()) {
            i++;
            if(mons == this)
                return(i);
        }
    }
    return(0);
}

//***********************************************************************
//                      getRandomMonster
//***********************************************************************

Creature *getRandomMonster(BaseRoom *inRoom) {
    Creature *foundCrt=nullptr;
    int         count=0, roll=0, num=0;

    num = inRoom->countCrt();
    if(!num)
        return(nullptr);

    roll = Random::get(1, num);
    for(Monster* mons : inRoom->monsters) {
        if(mons->isPet())
            continue;
        if(++count == roll) {
            foundCrt = mons;
            break;
        }
    }

    if(foundCrt)
        return(foundCrt);
    return(nullptr);
}

//***********************************************************************
//                      getRandomPlayer
//***********************************************************************

Creature *getRandomPlayer(BaseRoom *inRoom) {
    Creature *foundPly=nullptr;
    int         count=0, roll=0, num=0;

    num = inRoom->countVisPly();
    if(!num)
        return(nullptr);
    roll = Random::get(1, num);
    for(Player* ply : inRoom->players) {
        if(ply->flagIsSet(P_DM_INVIS)) {
            continue;
        }
        count++;
        if(count == roll) {
            foundPly = ply;
            break;
        }
    }

    if(foundPly)
        return(foundPly);
    return(nullptr);
}


//***********************************************************************
//                      validateAc
//***********************************************************************

void Monster::validateAc() {
    unsigned int ac=0;

    ac = get_perm_ac(level);
    if(armor > ac)
        armor = ac;
}

//***********************************************************************
//                      doHarmfulAuras
//***********************************************************************

int Monster::doHarmfulAuras() {
    int         a=0,dmg=0,aura=0, saved=0;
    long        i=0,t=0;
    BaseRoom    *inRoom=nullptr;
    Creature* player=nullptr;

    if(isPet())
        return(0);
    if(hp.getCur() < hp.getMax()/10)
        return(0);
    if(flagIsSet(M_CHARMED))
        return(0);

    for(a=0;a<MAX_AURAS;a++) {
        if(flagIsSet(M_FIRE_AURA + a))
            aura++;
    }

    if(!aura)
        return(0);

    i = lasttime[LT_M_AURA_ATTACK].ltime;
    t = time(nullptr);

    if(t - i < 20L) // Mob has to wait 20 seconds.
        return(0);
    else {
        lasttime[LT_M_AURA_ATTACK].ltime = t;
        lasttime[LT_M_AURA_ATTACK].interval = 20L;
    }

    inRoom = getRoomParent();
    if(!inRoom)
        return(0);

    for(a=0;a<MAX_AURAS;a++) {

        if(!flagIsSet(M_FIRE_AURA + a))
            continue;
        auto pIt = inRoom->players.begin();
        auto pEnd = inRoom->players.end();
        while(pIt != pEnd) {
            player = (*pIt++);

            if(player->isEffected("petrification") || player->isCt())
                continue;

            dmg = Random::get(level/2, (level*3)/2);

            dmg = MAX(2,dmg);

            switch(a+M_FIRE_AURA) {
            case M_FIRE_AURA:
                if(player->isEffected("heat-protection") || player->isEffected("alwayswarm"))
                    continue;

                saved = player->chkSave(BRE, this, 0);
                if(saved)
                    dmg /=2;
                player->printColor("^R%M's firey aura singes you for %s%d^R damage!\n", this, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
                break;
            case M_COLD_AURA:
                if(player->isEffected("warmth") || player->isEffected("alwayscold"))
                    continue;

                saved = player->chkSave(BRE, this, 0);
                if(saved)
                    dmg /=2;
                player->printColor("^C%M's freezing aura chills you for %s%d^C damage!\n", this, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
                break;
            case M_NAUSEATING_AURA:
                if(player->immuneToPoison())
                    continue;

                saved = player->chkSave(POI, this, 0);
                if(saved)
                    dmg /=2;
                player->printColor("^g%M's foul stench chokes and nauseates you for %s%d^g damage.\n", this, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
                break;
            case M_NEGATIVE_LIFE_AURA:
                if(player->isUndead() || player->isEffected("drain-shield"))
                    continue;

                saved = player->chkSave(DEA, this, 0);
                if(saved)
                    dmg /=2;
                player->printColor("^m%M's negative aura taps your life for %s%d^m damage.\n", this, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
                break;
            case M_TURBULENT_WIND_AURA:
                if(player->isEffected("wind-protection"))
                    continue;

                saved = player->chkSave(BRE, this, 0);
                if(saved)
                    dmg /=2;
                player->printColor("^W%M's turbulent winds buff you about for %s%d^W damage.\n", this, player->customColorize("*CC:DAMAGE*").c_str(), dmg);
                break;
            }

            player->hp.decrease(dmg);
            if(player->checkDie(this))
                return(1);

        }// End while
    }// End for

    return(0);
}


//***********************************************************************
//                      isGuardLoot
//***********************************************************************

bool isGuardLoot(BaseRoom *inRoom, Creature* player, const char *fmt) {
    for(Monster* mons : inRoom->monsters) {
        if(mons->flagIsSet(M_GUARD_TREATURE) && !player->checkStaff(fmt, mons))
            return(true);
    }
    return(false);
}


//*********************************************************************
//                      race aggro
//*********************************************************************

bool Monster::isRaceAggro(int x, bool checkInvert) const {
    x--;
    bool set = (raceAggro[x/8] & 1<<(x%8));
    if(!checkInvert)
        return(set);
    return(set ? !flagIsSet(M_RACE_AGGRO_INVERT) : flagIsSet(M_RACE_AGGRO_INVERT));
}
void Monster::setRaceAggro(int x) {
    x--;
    raceAggro[x/8] |= 1<<(x%8);
}
void Monster::clearRaceAggro(int x) {
    x--;
    raceAggro[x/8] &= ~(1<<(x%8));
}


//*********************************************************************
//                      class aggro
//*********************************************************************

bool Monster::isClassAggro(int x, bool checkInvert) const {
    x--;
    bool set = (cClassAggro[x/8] & 1<<(x%8));
    if(!checkInvert)
        return(set);
    return(set ? !flagIsSet(M_CLASS_AGGRO_INVERT) : flagIsSet(M_CLASS_AGGRO_INVERT));
}
void Monster::setClassAggro(int x) {
    x--;
    cClassAggro[x/8] |= 1<<(x%8);
}
void Monster::clearClassAggro(int x) {
    x--;
    cClassAggro[x/8] &= ~(1<<(x%8));
}


//*********************************************************************
//                      deity aggro
//*********************************************************************

bool Monster::isDeityAggro(int x, bool checkInvert) const {
    x--;
    bool set = (deityAggro[x/8] & 1<<(x%8));
    if(!checkInvert)
        return(set);
    return(set ? !flagIsSet(M_DEITY_AGGRO_INVERT) : flagIsSet(M_DEITY_AGGRO_INVERT));
}
void Monster::setDeityAggro(int x) {
    x--;
    deityAggro[x/8] |= 1<<(x%8);
}
void Monster::clearDeityAggro(int x) {
    x--;
    deityAggro[x/8] &= ~(1<<(x%8));
}

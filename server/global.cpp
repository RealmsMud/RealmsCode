/*
 * global.cpp
 *   Global variables and such
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

#define MIGNORE

#include "dice.hpp"      // for Dice
#include "global.hpp"    // for SPL, DEA, POI, MEN, BRE, CreatureClass, Crea...
#include "lasttime.hpp"  // for lasttime
#include "magic.hpp"     // for S_ATROPHY, S_BLISTER, S_BLIZZARD, S_BLOODBOIL
#include "mud.hpp"       // for SONG_DESTRUCTION, SONG_MASS_DESTRUCTION
#include "realm.hpp"     // for NO_REALM, COLD, EARTH, ELEC, FIRE, WATER, WIND
#include "structs.hpp"   // for osong_t, osp_t

char questions_to_email[80]="realms@rohonline.net";

long InBytes = 0;
long UnCompressedBytes = 1; // So we never have a divide by 0 error ;)
long OutBytes = 0;

// How many times has crash been called?
int  Crash = 0;

// Numbers of supports required to create a guild
int SUPPORT_REQUIRED = 2;

int     Tablesize;
long    StartTime;
struct  lasttime    Shutdown;
struct  lasttime    Weather[5];

const char *dmname[] = {
    "Bane", "Dominus", "Ocelot", "Kriona", "Nikola", nullptr
};




short saving_throw_cycle[][10] = { // POI   DEA   BRE   MEN   SPL
        // 2    3   4   5   6   7   8   9   10   11
        { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0   },
        { POI, DEA, BRE, POI, SPL, MEN, POI, DEA, BRE, POI },  // assassin
        { DEA, BRE, MEN, SPL, DEA, BRE, POI, DEA, POI, SPL },  // barbarian
        { SPL, MEN, DEA, POI, SPL, BRE, SPL, MEN, BRE, SPL },  // cleric
        { MEN, BRE, MEN, SPL, DEA, BRE, POI, DEA, POI, BRE },  // fighter
        { SPL, POI, MEN, SPL, DEA, MEN, SPL, BRE, MEN, SPL },  // mage
        { BRE, MEN, DEA, POI, SPL, BRE, SPL, POI, BRE, DEA },  // paladin
        { SPL, BRE, MEN, POI, DEA, BRE, MEN, DEA, POI, SPL },  // ranger
        { POI, MEN, DEA, POI, SPL, DEA, POI, DEA, BRE, POI },  // thief
        { SPL, MEN, DEA, SPL, BRE, MEN, SPL, MEN, DEA, POI },  // pureblood
        { POI, MEN, SPL, BRE, DEA, MEN, POI, MEN, SPL, MEN },  // monk
        { BRE, MEN, DEA, POI, SPL, BRE, SPL, POI, BRE, DEA },  // death-knight
        { POI, SPL, POI, DEA, MEN, BRE, SPL, POI, DEA, POI },  // druid
        { SPL, DEA, MEN, SPL, DEA, MEN, SPL, MEN, DEA, SPL },  // lich
        { DEA, BRE, POI, MEN, DEA, BRE, SPL, DEA, POI, SPL },  // werewolf
        { SPL, MEN, DEA, SPL, BRE, POI, MEN, POI, SPL, BRE },  // bard
        { DEA, MEN, POI, DEA, SPL, POI, DEA, POI, BRE, DEA },  // rogue
        { POI, DEA, BRE, MEN, SPL, POI, DEA, BRE, MEN, SPL },  // builder
        { POI, DEA, BRE, MEN, SPL, POI, DEA, BRE, MEN, SPL },  //   unused
        { POI, DEA, BRE, MEN, SPL, POI, DEA, BRE, MEN, SPL },  // caretaker
        { POI, DEA, BRE, MEN, SPL, POI, DEA, BRE, MEN, SPL }   // dungeonmaster
};


struct osong_t osong[] = {
    { SONG_DESTRUCTION,     Dice(10, 20, 10) },
    { SONG_MASS_DESTRUCTION,    Dice(5,  10, 5) },
    { -1,                   Dice(0,  0,  0) }
};

struct osp_t ospell[] = {
    /*
    int splno;
    char    realm;
    Dice    damage;
    char    bonus_type;
    bool    drain
    */
    { S_RUMBLE,         EARTH,  3,  Dice(1, 8, 0),  1, false},  // rumble
    { S_ZEPHYR,         WIND,   3,  Dice(1, 8, 0),  1, false},  // hurt
    { S_BURN,           FIRE,   3,  Dice(1, 8, 0),  1, false},  // burn
    { S_BLISTER,        WATER,  3,  Dice(1, 8, 0),  1, false},  // blister
    { S_ZAP,            ELEC,   3,  Dice(1, 8, 0),  1, false},  // zap
    { S_CHILL,          COLD,   3,  Dice(1, 8, 0),  1, false},  // chill

    { S_CRUSH,          EARTH,  7,  Dice(2, 5, 7),  2, false},  // crush
    { S_DUSTGUST,       WIND,   7,  Dice(2, 5, 7),  2, false},  // dustgust 9-17 base dmg
    { S_FIREBALL,       FIRE,   7,  Dice(2, 5, 7),  2, false},  // fireball
    { S_WATERBOLT,      WATER,  7,  Dice(2, 5, 7),  2, false},  // waterbolt
    { S_LIGHTNING_BOLT, ELEC,   7,  Dice(2, 5, 7),  2, false},  // lightning-bolt
    { S_FROSTBITE,      COLD,   7,  Dice(2, 5, 7),  2, false},  // frostbite

    { S_ENGULF,         EARTH,  10, Dice(2, 5, 14), 2, false},  // ACTUALLY SHATTERSTONE
    { S_WHIRLWIND,      WIND,   10, Dice(2, 5, 14), 2, false},  // whirlwind 16-24 base damage
    { S_BURSTFLAME,     FIRE,   10, Dice(2, 5, 14), 2, false},  // burstflame
    { S_STEAMBLAST,     WATER,  10, Dice(2, 5, 14), 2, false},  // steamblast
    { S_SHOCKBOLT,      ELEC,   10, Dice(2, 5, 14), 2, false},  // shockbolt
    { S_SLEET,          COLD,   10, Dice(2, 5, 14), 2, false},  // sleet

    { S_SHATTERSTONE,   EARTH,  15, Dice(3, 4, 21), 3, false},  // ACTUALLY ENGULF
    { S_CYCLONE,        WIND,   15, Dice(3, 4, 21), 3, false},  // cyclone 24-35 base damage
    { S_IMMOLATE,       FIRE,   15, Dice(3, 4, 21), 3, false},  // immolate
    { S_BLOODBOIL,      WATER,  15, Dice(3, 4, 21), 3, false},  // bloodboil
    { S_ELECTROCUTE,    ELEC,   15, Dice(3, 4, 21), 3, false},  // electrocute
    { S_COLD_CONE,      COLD,   15, Dice(3, 4, 21), 3, false},  // cold-cone

    { S_EARTH_TREMOR,   EARTH,  25, Dice(4, 5, 35), 3, false},  // earth-tremor
    { S_TEMPEST,        WIND,   25, Dice(4, 5, 35), 3, false},  // tempest 39-59 base damage
    { S_FLAMEFILL,      FIRE,   25, Dice(4, 5, 35), 3, false},  // flamefill
    { S_ATROPHY,        WATER,  25, Dice(4, 5, 35), 3, false},  // atrophy
    { S_THUNDERBOLT,    ELEC,   25, Dice(4, 5, 35), 3, false},  // thuderbolt
    { S_ICEBLADE,       COLD,   25, Dice(4, 5, 35), 3, false},  // iceblade

    // multiple target spells
    { S_EARTHQUAKE,         EARTH,  3,  Dice(2, 5, 10), 2, false},  // earthquake
    { S_HURRICANE,          WIND,   3,  Dice(2, 5, 10), 2, false},  // hurricane 12-20 base damage
    { S_FIRESTORM,          FIRE,   3,  Dice(2, 5, 10), 2, false},  // firestorm
    { S_FLOOD,              WATER,  3,  Dice(2, 5, 10), 2, false},  // tsunami
    { S_CHAIN_LIGHTNING,    ELEC,   3,  Dice(2, 5, 10), 2, false},  // chain-lightning
    { S_BLIZZARD,           COLD,   3,  Dice(2, 5, 10), 2, false},  // icestorm

    // necro spells
    { S_SAP_LIFE,           NO_REALM,   3,  Dice(2, 4, 0),      0, true},
    { S_LIFETAP,            NO_REALM,   6,  Dice(2, 5, 8),      0, true},
    { S_LIFEDRAW,           NO_REALM,   9,  Dice(2, 6, 18),     0, true},
    { S_DRAW_SPIRIT,        NO_REALM,   12, Dice(2, 7, 30),     0, true},
    { S_SIPHON_LIFE,        NO_REALM,   15, Dice(2, 8, 44),     0, true},
    { S_SPIRIT_STRIKE,      NO_REALM,   18, Dice(2, 9, 60),     0, true},
    { S_SOULSTEAL,          NO_REALM,   21, Dice(2, 10, 78),    0, true},
    { S_TOUCH_OF_KESH,      NO_REALM,   24, Dice(2, 11, 98),    0, true},

    { -1, NO_REALM, 0, Dice(0, 0, 0), 0 }
};



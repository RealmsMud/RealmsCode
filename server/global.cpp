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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#define MIGNORE

#include "magic.hpp"
#include "mud.hpp"

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


char allowedClassesStr[static_cast<int>(CreatureClass::CLASS_COUNT) + 4][16] = { "Assassin", "Berserker", "Cleric", "Fighter",
            "Mage", "Paladin", "Ranger", "Thief", "Pureblood", "Monk", "Death Knight",
            "Druid", "Lich", "Werewolf", "Bard", "Rogue", "Figh/Mage", "Figh/Thief",
            "Cler/Ass", "Mage/Thief", "Thief/Mage", "Cler/Figh", "Mage/Ass" };


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

int statBonus[MAXALVL] = {
    -4, -4, -4,         // 0 - 2
    -3, -3,             // 3 - 4
    -2, -2,             // 5 - 6
    -1,                 // 7
    0, 0, 0, 0, 0, 0,   // 8 - 13
    1, 1, 1,            // 14 - 16
    2, 2, 2, 2,         // 17 - 20
    3, 3, 3, 3,         // 21 - 24
    4, 4, 4,            // 25 - 27
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 };   // 28+


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

int numQuests = 0;


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
    */
    { S_RUMBLE,         EARTH,  3,  Dice(1, 8, 0),  1, 0},  // rumble
    { S_ZEPHYR,         WIND,   3,  Dice(1, 8, 0),  1, 0},  // hurt
    { S_BURN,           FIRE,   3,  Dice(1, 8, 0),  1, 0},  // burn
    { S_BLISTER,        WATER,  3,  Dice(1, 8, 0),  1, 0},  // blister
    { S_ZAP,            ELEC,   3,  Dice(1, 8, 0),  1, 0},  // zap
    { S_CHILL,          COLD,   3,  Dice(1, 8, 0),  1, 0},  // chill

    { S_CRUSH,          EARTH,  7,  Dice(2, 5, 7),  2, 0},  // crush
    { S_DUSTGUST,       WIND,   7,  Dice(2, 5, 7),  2, 0},  // dustgust 9-17 base dmg
    { S_FIREBALL,       FIRE,   7,  Dice(2, 5, 7),  2, 0},  // fireball
    { S_WATERBOLT,      WATER,  7,  Dice(2, 5, 7),  2, 0},  // waterbolt
    { S_LIGHTNING_BOLT, ELEC,   7,  Dice(2, 5, 7),  2, 0},  // lightning-bolt
    { S_FROSTBITE,      COLD,   7,  Dice(2, 5, 7),  2, 0},  // frostbite

    { S_ENGULF,         EARTH,  10, Dice(2, 5, 14), 2, 0},  // ACTUALLY SHATTERSTONE
    { S_WHIRLWIND,      WIND,   10, Dice(2, 5, 14), 2, 0},  // whirlwind 16-24 base damage
    { S_BURSTFLAME,     FIRE,   10, Dice(2, 5, 14), 2, 0},  // burstflame
    { S_STEAMBLAST,     WATER,  10, Dice(2, 5, 14), 2, 0},  // steamblast
    { S_SHOCKBOLT,      ELEC,   10, Dice(2, 5, 14), 2, 0},  // shockbolt
    { S_SLEET,          COLD,   10, Dice(2, 5, 14), 2, 0},  // sleet

    { S_SHATTERSTONE,   EARTH,  15, Dice(3, 4, 21), 3, 0},  // ACTUALLY ENGULF
    { S_CYCLONE,        WIND,   15, Dice(3, 4, 21), 3, 0},  // cyclone 24-35 base damage
    { S_IMMOLATE,       FIRE,   15, Dice(3, 4, 21), 3, 0},  // immolate
    { S_BLOODBOIL,      WATER,  15, Dice(3, 4, 21), 3, 0},  // bloodboil
    { S_ELECTROCUTE,    ELEC,   15, Dice(3, 4, 21), 3, 0},  // electrocute
    { S_COLD_CONE,      COLD,   15, Dice(3, 4, 21), 3, 0},  // cold-cone

    { S_EARTH_TREMOR,   EARTH,  25, Dice(4, 5, 35), 3, 0},  // earth-tremor
    { S_TEMPEST,        WIND,   25, Dice(4, 5, 35), 3, 0},  // tempest 39-59 base damage
    { S_FLAMEFILL,      FIRE,   25, Dice(4, 5, 35), 3, 0},  // flamefill
    { S_ATROPHY,        WATER,  25, Dice(4, 5, 35), 3, 0},  // atrophy
    { S_THUNDERBOLT,    ELEC,   25, Dice(4, 5, 35), 3, 0},  // thuderbolt
    { S_ICEBLADE,       COLD,   25, Dice(4, 5, 35), 3, 0},  // iceblade

    // multiple target spells
    { S_EARTHQUAKE,         EARTH,  3,  Dice(2, 5, 10), 2, 0},  // earthquake
    { S_HURRICANE,          WIND,   3,  Dice(2, 5, 10), 2, 0},  // hurricane 12-20 base damage
    { S_FIRESTORM,          FIRE,   3,  Dice(2, 5, 10), 2, 0},  // firestorm
    { S_FLOOD,              WATER,  3,  Dice(2, 5, 10), 2, 0},  // tsunami
    { S_CHAIN_LIGHTNING,    ELEC,   3,  Dice(2, 5, 10), 2, 0},  // chain-lightning
    { S_BLIZZARD,           COLD,   3,  Dice(2, 5, 10), 2, 0},  // icestorm

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


// Wolf leveling code
Dice wolf_dice[41] =
{                           // Old dice
    Dice(2, 2, 0),   /* 0  */  // Dice(1, 4, 0)
    Dice(2, 2, 0),   /* 1  */  // Dice(1, 4, 0)
    Dice(2, 2, 1),   /* 2  */  // Dice(1, 5, 1)
    Dice(2, 3, 1),   /* 3  */  // Dice(1, 7, 1)
    Dice(2, 4, 0),   /* 4  */  // Dice(1, 7, 2)
    Dice(3, 3, 0),   /* 5  */  // Dice(2, 3, 2)
    Dice(3, 3, 2),   /* 6  */  // Dice(2, 4, 1)
    Dice(4, 3, 0),   /* 7  */  // Dice(2, 4, 2)
    Dice(4, 3, 1),   /* 8  */  // Dice(2, 5, 1)
    Dice(5, 3, 0),   /* 9  */  // Dice(2, 5, 2)
    Dice(5, 3, 1),   /* 10 */  // Dice(2, 6, 1)
    Dice(5, 3, 2),   /* 11 */  // Dice(2, 6, 2)
    Dice(6, 3, 1),   /* 12 */  // Dice(3, 4, 1)
    Dice(6, 3, 2),   /* 13 */  // Dice(3, 4, 2)
    Dice(7, 3, 0),   /* 14 */  // Dice(3, 5, 1)
    Dice(7, 3, 1),   /* 15 */  // Dice(3, 5, 2)
    Dice(7, 3, 2),   /* 16 */  // Dice(3, 7, 1)
    Dice(7, 3, 3),   /* 17 */  // Dice(4, 5, 0)
    Dice(7, 4, 0),   /* 18 */  // Dice(5, 6, 1)
    Dice(7, 4, 2),   /* 19 */  // Dice(5, 6, 2)
    Dice(7, 4, 3),   /* 20 */  // Dice(6, 5, 3)
    Dice(7, 4, 5),   /* 21 */  // Dice(6, 6, 0)
    Dice(7, 5, 0),   /* 22 */  // Dice(6, 6, 2)
    Dice(7, 5, 2),   /* 23 */  // Dice(6, 6, 3)
    Dice(7, 5, 1),   /* 24 */  // Dice(6, 7, 1)
    Dice(7, 5, 3),   /* 25 */  // Dice(6, 8, 0)
    Dice(8, 5, 0),   /* 26 */  // Dice(6, 8, 2)
    Dice(8, 5, 2),   /* 27 */  // Dice(6, 8, 4)
    Dice(8, 5, 4),   /* 28 */  // Dice(7, 7, 2)
    Dice(9, 5, 2),   /* 29 */  // Dice(7, 7, 4)
    Dice(9, 5, 3),  /* 30 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 31 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 32 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 33 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 34 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 35 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 36 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 37 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 38 */  // Dice(7, 7, 6)
    Dice(9, 5, 3),  /* 39 */  // Dice(7, 7, 6)
    Dice(9, 5, 3)   /* 40 */  // Dice(7, 7, 6)
};


// monk leveling code
Dice monk_dice[41] =
{
    Dice(1, 3, 0),  /* 0  */  // Old dice
    Dice(1, 3, 0),  /* 1  */  // Dice(1, 3, 0)
    Dice(1, 4, 0),  /* 2  */  // Dice(1, 5, 0)
    Dice(1, 5, 0),  /* 3  */  // Dice(1, 5, 1)
    Dice(1, 6, 0),  /* 4  */  // Dice(1, 6, 0)
    Dice(1, 6, 1),  /* 5  */  // Dice(1, 6, 1)
    Dice(2, 4, 1),  /* 6  */  // Dice(1, 6, 2)
    Dice(2, 5, 0),  /* 7  */  // Dice(2, 3, 1)
    Dice(2, 5, 1),  /* 8  */  // Dice(2, 4, 0)
    Dice(2, 6, 0),  /* 9  */  // Dice(2, 4, 1)
    Dice(2, 6, 2),  /* 10 */  // Dice(2, 5, 0)
    Dice(3, 5, 2),  /* 11 */  // Dice(2, 5, 2)
    Dice(3, 6, 0),  /* 12 */  // Dice(2, 6, 1)
    Dice(3, 6, 2),  /* 13 */  // Dice(2, 6, 2)
    Dice(3, 7, 0),  /* 14 */  // Dice(3, 6, 1)
    Dice(3, 7, 2),  /* 15 */  // Dice(3, 7, 1)
    Dice(4, 6, 2),  /* 16 */  // Dice(4, 7, 1)
    Dice(4, 7, 0),  /* 17 */  // Dice(5, 7, 0)
    Dice(4, 7, 2),  /* 18 */  // Dice(5, 8, 1)
    Dice(4, 8, 0),  /* 19 */  // Dice(6, 7, 0)
    Dice(4, 8, 2),  /* 20 */  // Dice(6, 7, 2)
    Dice(5, 7, 2),  /* 21 */  // Dice(6, 8, 0)
    Dice(5, 8, 0),  /* 22 */  // Dice(6, 8, 2)
    Dice(5, 8, 2),  /* 23 */  // Dice(6, 9, 0)
    Dice(5, 9, 0),  /* 24 */  // Dice(6, 9, 2)
    Dice(5, 9, 2),  /* 25 */  // Dice(6, 10, 0 )
    Dice(6, 8, 2),  /* 26 */  // Dice(6, 10, 2 )
    Dice(6, 9, 0),  /* 27 */  // Dice(7, 9, 4)
    Dice(6, 9, 2),  /* 28 */  // Dice(7, 9, 6)
    Dice(6, 10, 0), /* 29 */  // Dice(7, 8, 8)
    Dice(6, 10, 2), /* 30 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 31 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 32 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 33 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 34 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 35 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 36 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 37 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 38 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2), /* 39 */  // Dice(8, 8, 10 )
    Dice(6, 10, 2)  /* 40 */  // Dice(8, 8, 10 )

};



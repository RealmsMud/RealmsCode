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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#define MIGNORE

#include "creatures.h"
#include "magic.h"
#include "mud.h"

/*
 *
 * Configurable settings.
 *
 */
char auth_questions_email[80]="realms@rohonline.net";
char questions_to_email[80]="realms@rohonline.net";
char register_questions_email[80]="realms@rohonline.net";

long InBytes = 0;
long UnCompressedBytes = 1; // So we never have a divide by 0 error ;)
long OutBytes = 0;
int bHavePort = 0;

// How many times has crash been called?
int  Crash = 0;

// Numbers of supports required to create a guild
int SUPPORT_REQUIRED = 2;

int     Tablesize;
int     Cmdnum;
long    StartTime;
struct  lasttime    Shutdown;
struct  lasttime    Weather[5];
int     Numlockedout;

const char *dmname[] = {
    "Bane", "Dominus", "Ocelot", "Kriona", nullptr
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

int statBonus[40] = {
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


short multiHpMpAdj[MAX_MULTICLASS][2] = {
    //  Hp  Mp     ID  Classes
    {   0,  0   }, // 0: None
    {   5,  4   }, // 1: Fighter/Mage
    {   6,  2   }, // 2: Fighter/Thief
    {   5,  3   }, // 3: Cleric/Assassin
    {   5,  4   }, // 4: Mage/Thief
    {   5,  3   }, // 5: Thief/Mage
    {   6,  3   }, // 6: Cleric/Fighter
    {   5,  3   }  // 7: Mage/Assassin
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

int numBans = 0;
int numQuests = 0;
//int maxGuild = 0;

unsigned long needed_exp[] = {
    //2   3  4   5   6   7    8   9   10
    500, 1000, 2000, 4000, 8000, 16000, 32000, 64000, 100000,
    // 500  1k   2k   4k     8k 16k 32k   36k
    //11      12      13      14      15      16       17
    160000, 270000, 390000, 560000, 750000, 1000000, 1300000,
    // 60k   110k   120k   170k 190k    250k     300k
    //18      19        20     21      22      23      24
    1700000, 2200000, 2800000, 3500000, 4300000, 5300000, 6500000,
    //400k   500k   600k     700k    800k    1mil    1.5mil
    // 25      26     27        28        29       30        31
    8000000, 10000000, 12200000, 14700000, 17500000, 21500000, 25700000,
    // 32       33     34      35        36     37      38
    30100000, 34700000, 39600000, 44800000, 50300000, 56200000, 62500000,
    // 39       40      41
    69200000, 76200000, 2000000000

    };




char article[][10] = {
    "the",
    "from",
    "to",
    "with",
    "an",
    "in",
    "for",
    "@"
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


//typedef struct {
//  short   hp;
//  short   mp;
//  short   armor;
//  short   thaco;
//  short   ndice;
//  short   sdice;
//  short   pdice;
//  short   str;
//  short   dex;
//  short   con;
//  short   intel;
//  short   pie;
//  long    realms;
//} creatureStats;
//
    // 0 = weak, 1 = normal, 2 = buff
creatureStats conjureStats[3][40]  =
{
    // Weak
   {
        {25,4,100,21,1,2,0,16,10,12,10,10,0},
        {29,8,110,20,1,6,0,16,10,12,10,10,250},
        {33,12,120,19,1,7,0,16,10,14,10,10,500},
        {37,16,130,18,2,2,2,16,10,15,10,10,750},
        {41,20,140,17,2,4,0,16,10,16,10,10,1250},
        {45,24,150,16,2,4,4,16,10,16,11,10,1500},
        {49,28,160,15,2,3,5,16,10,16,12,10,3000},
        {53,32,170,14,3,4,2,16,10,16,13,10,6000},
        {57,36,180,13,3,5,1,16,10,16,14,12,11250},
        {61,40,190,12,4,4,2,16,10,16,15,12,13250},
        {65,44,200,11,3,6,1,16,10,16,16,12,15000},
        {69,48,210,10,2,9,6,16,10,16,16,13,17500},
        {73,52,220,9,4,4,3,16,10,16,16,14,20000},
        {77,56,230,8,4,5,3,16,10,16,17,14,22500},
        {81,60,240,7,4,5,5,16,10,16,18,16,25000},
        {85,64,250,6,4,5,7,16,10,17,18,17,27500},
        {89,68,260,5,4,5,9,18,10,18,19,18,30000},
        {93,72,270,4,5,6,4,18,10,18,20,18,32500},
        {97,76,280,3,5,6,8,18,10,18,20,19,35000},
        {101,80,290,2,4,7,8,18,10,18,20,19,37500},
        {105,84,300,1,9,5,1,18,10,18,20,19,40000},
        {109,88,310,0,8,5,8,19,10,19,20,19,47500},
        {113,92,320,-1,10,5,2,19,10,19,20,19,55000},
        {117,96,330,-2,10,5,5,19,10,19,20,19,62500},
        {121,100,340,-3,20,3,0,20,10,19,20,20,70000},
        {125,104,350,-4,20,3,2,20,10,19,20,20,77500},
        {129,108,360,-5,13,4,9,20,10,20,20,20,85000},
        {133,112,370,-6,13,5,2,20,10,20,20,20,92500},
        {137,116,380,-7,15,4,6,21,12,20,21,20,100000},
        {141,120,390,-8,16,3,13,21,12,21,21,21,107500},
        {145,124,400,-9,16,3,15,22,13,22,22,22,120000},
        {149,128,410,-10,16,3,17,23,13,22,22,22,130000},
        {153,132,420,-11,16,3,19,23,14,23,22,22,140000},
        {157,136,430,-12,16,3,21,23,14,23,23,23,150000},
        {161,140,440,-13,16,3,23,24,15,23,23,23,165000},
        {165,144,450,-14,16,3,25,24,15,23,24,23,180000},
        {169,148,460,-15,16,3,27,24,16,24,24,24,195000},
        {173,152,470,-16,16,3,29,25,17,24,24,25,210000},
        {177,156,480,-17,16,3,31,25,18,25,25,25,230000},
        {181,160,490,-18,16,3,33,26,19,25,26,25,250000}
    },
    // Normal
    {
        {27,5,200,20,1,4,0,16,10,10,10,10,0},
        {32,10,215,19,2,3,0,17,10,12,10,10,500},
        {37,15,230,18,2,3,2,18,10,14,10,10,1000},
        {42,20,245,17,2,3,2,18,10,15,10,10,1500},
        {47,25,260,16,2,3,4,18,10,16,10,10,2500},
        {52,30,275,15,4,2,4,18,10,17,12,10,3000},
        {57,35,290,14,2,4,6,18,10,18,14,10,6000},
        {62,40,305,13,4,3,4,18,10,18,15,10,12000},
        {67,45,320,12,5,3,3,18,10,18,16,10,22500},
        {72,50,335,11,3,5,5,18,10,18,17,10,26500},
        {77,55,350,10,3,6,4,18,10,18,18,10,30000},
        {82,60,365,9,3,6,6,18,10,18,18,12,35000},
        {87,65,380,8,4,4,8,18,10,18,18,14,40000},
        {92,70,395,7,4,5,8,18,10,18,18,15,45000},
        {97,75,410,6,5,5,5,18,10,18,18,16,50000},
        {102,80,425,5,4,6,8,18,10,18,18,17,55000},
        {107,85,440,4,4,6,9,18,10,18,18,18,60000},
        {112,90,465,3,5,6,8,19,10,19,19,19,65000},
        {117,95,480,2,6,5,10,20,10,20,20,20,70000},
        {122,100,495,1,4,7,10,21,10,20,20,20,75000},
        {127,105,510,0,9,5,4,21,11,21,20,20,80000},
        {132,110,525,-2,8,5,10,21,11,21,21,20,95000},
        {137,115,540,-4,10,5,6,21,11,21,21,21,110000},
        {142,120,555,-7,10,5,8,21,11,22,21,21,125000},
        {147,125,570,-8,15,4,0,21,11,22,22,21,140000},
        {152,130,585,-8,20,3,4,21,11,22,22,21,155000},
        {157,135,600,-8,13,4,12,21,11,22,22,21,170000},
        {162,140,615,-8,13,5,5,22,11,22,23,22,185000},
        {167,145,630,-9,15,4,10,22,11,23,23,22,200000},
        {172,150,645,-10,18,3,16,23,11,23,23,23,215000},
        {177,155,660,-10,18,3,19,23,11,23,23,23,230000},
        {182,160,675,-11,18,3,22,24,13,24,24,23,250000},
        {187,165,690,-12,18,3,25,24,14,24,24,24,275000},
        {192,170,705,-13,18,3,28,25,15,25,25,24,300000},
        {197,175,720,-14,18,3,31,25,16,25,25,25,325000},
        {202,180,735,-15,18,3,34,26,17,26,26,25,350000},
        {207,185,750,-16,18,3,37,26,18,26,26,26,375000},
        {212,190,765,-17,18,3,40,27,19,27,27,26,410000},
        {217,195,780,-18,18,3,43,27,20,27,27,27,450000},
        {222,200,795,-19,18,3,46,28,21,28,28,27,490000}
    },
    // Buff
       {
        {30,6,300,20,2,2,1,16,12,10,10,10,0},
        {36,12,320,19,3,2,0,16,12,10,10,10,1000},
        {42,18,340,18,3,2,3,17,13,12,10,10,2000},
        {48,24,360,16,3,3,3,18,13,14,10,10,3000},
        {54,30,380,15,3,2,6,18,13,15,10,10,5000},
        {60,36,400,14,4,2,7,18,14,16,10,10,6000},
        {66,42,420,13,4,2,7,18,14,17,12,10,12000},
        {72,48,440,12,4,3,6,18,15,18,14,10,24000},
        {78,54,460,11,4,4,4,18,15,18,14,10,45000},
        {84,60,480,10,5,3,4,18,15,18,14,10,53000},
        {90,66,500,9,6,3,3,18,15,18,15,10,60000},
        {96,72,520,8,3,6,8,18,16,18,16,10,70000},
        {102,78,540,7,4,4,11,18,16,18,16,10,80000},
        {108,84,560,6,5,4,9,18,17,18,18,10,90000},
        {114,90,580,5,6,5,6,18,17,18,18,12,100000},
        {120,96,600,4,4,6,10,18,18,18,18,14,110000},
        {126,102,620,3,5,6,10,18,18,18,18,15,120000},
        {132,108,640,2,5,6,10,18,19,18,18,16,130000},
        {138,114,660,1,6,6,10,18,19,18,18,17,140000},
        {144,120,680,0,7,4,11,18,20,18,18,18,150000},
        {150,126,700,-1,9,5,7,18,20,19,19,19,160000},
        {156,132,720,-3,8,5,14,20,21,20,20,20,190000},
        {162,138,740,-5,10,5,9,21,21,20,21,20,220000},
        {168,144,760,-8,10,5,11,22,22,21,22,20,250000},
        {174,150,780,-9,12,5,0,23,23,22,23,21,280000},
        {180,156,800,-9,20,3,6,23,23,22,23,21,310000},
        {186,162,820,-10,13,4,16,23,23,22,23,22,340000},
        {192,168,840,-10,13,5,9,23,23,23,23,23,370000},
        {198,174,860,-10,15,4,14,24,24,24,24,24,400000},
        {204,180,880,-11,15,4,20,25,25,25,25,25,430000},
        {210,186,900,-12,15,4,26,25,25,25,25,25,460000},
        {216,192,920,-13,15,4,32,26,26,26,26,26,500000},
        {222,198,940,-14,15,4,38,26,26,26,26,26,540000},
        {228,204,960,-15,15,4,44,27,27,27,27,27,580000},
        {234,210,980,-16,15,4,50,27,27,27,27,27,625000},
        {240,216,1000,-17,15,4,56,28,28,28,28,28,670000},
        {246,222,1020,-18,15,4,62,28,28,28,28,28,715000},
        {252,228,1040,-19,15,4,68,29,29,29,29,29,760000},
        {258,234,1060,-20,15,4,74,29,29,29,29,29,810000},
        {264,240,1080,-21,15,4,80,30,30,30,30,30,860000}
    }
};



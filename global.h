/*
 * global.h
 *   Global defines required by the rest of the program that don't depend on anything else
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

// Size of exp array, also highest you can train
const int MAXALVL = 40;
const unsigned int MAX_ARMOR = 2000;

// I/O buffer sizes
const int IBUFSIZE = 1024;
const int OBUFSIZ = 8192;

// merror() error types
const int FATAL = 1;
const int NONFATAL = 0;

const int DEFAULT_WEAPON_DELAY = 30;   // 3 seconds


const char CUSTOM_COLOR_DEFAULT = '#';
const int CUSTOM_COLOR_SIZE = 20;

const int BROADLOG = 1;

const int MAX_MULTICLASS = 8;
// how often (in seconds) players get saved
const int SAVEINTERVAL = 1200;

const int MAXTIMEOUT = 14400;

const int MAXPAWN = 25000;

const int MAX_QUEST = 128;

const int MAX_FACTION = 32;
const int MAX_DUELS = 10;
const int MAX_BUILDER_RANGE = 10;

// Save flags
const int PERMONLY = 1;
const int ALLITEMS = 0;

// Command status returns
const int DISCONNECT = 1;
const int PROMPT = 2;
const int DOPROMPT = 3;



// base monster number for various types of summoned monsters
const int BASE_ELEMENTAL = 801;
const int BASE_UNDEAD = 550;
const int COMMANDMAX = 6;

// Monster and object files sizes (in terms of monsters or objects)
const int MFILESIZE = 100;
const int OFILESIZE = 100;

// memory limits

const int RMAX = 20000; // Max number of these allowed to be created
const int MMAX = 20000;
const int OMAX = 20000;
const int PMAX = 1024;

const unsigned int RQMAX = 600;  // Max number of these allowed in memory
const unsigned int MQMAX = 200;  // at any one time
const unsigned int OQMAX = 200;



const int WIELDOBJ = 1;
const int SECONDOBJ = 2;
const int HOLDOBJ = 3;
const int SHIELDOBJ = 4;

//const int MAXSONG = 10;

const int MAX_AURAS = 6;    // Max mob aura attacks

// Spell casting types
//const int CAST = 0;
//const int SKILL = 1;    // Druid summon-elemental


// NPC Trades

enum NpcTrades {
    NOTRADE = 0,
    SMITHY = 1,
    BANKER = 2,
    ARMOROR = 3,
    WEAPONSMITH = 4,
    MERCHANT = 5,
    TRAINING_PERM = 6,
    MOBTRADE_COUNT
};


// Monetary transaction types
enum MonetaryTransactions {
    BUY = 0,
    SELL = 1
};

// Creature stats
enum CrtStats {
    STR = 1,
    DEX = 2,
    CON = 3,
    INT = 4,
    PTY = 5,
    CHA = 6
};
const int MAX_STAT = PTY;

const int MIN_STAT_NUM = 10;
const int MAX_STAT_NUM = MAXALVL*10;

// Saving throw types
enum SavingThrows {
    LCK = 0,  // Luck
    POI = 1,  // Poison
    DEA = 2,  // Death
    BRE = 3,  // Breath Weapons
    MEN = 4,  // Mental
    SPL = 5,  // Spells

    MAX_SAVE = 6
};
const int MAX_SAVE_COLOR = 11;


// Alignments
enum Alignments {
    BLOODRED = -3,
    REDDISH = -2,
    PINKISH = -1,
    NEUTRAL = 0,
    LIGHTBLUE = 1,
    BLUISH = 2,
    ROYALBLUE = 3
};
// Attack types
enum AttackTypes {
    PHYSICAL = 1,
    MAGICAL = 2,
    MENTAL = 3,
    NEGATIVE_ENERGY = 4,
    ABILITY = 5,
    MAGICAL_NEGATIVE = 6
};

// Modifier List
enum ModifierList {
    MSTR = 0,
    MDEX = 1,
    MCON = 2,
    MINT = 3,
    MPIE = 4,
    MCHA = 5,
    MHP = 6,
    MMP = 7,
    MLCK = 8,
    MPOI = 9,
    MDEA = 10,
    MBRE = 11,
    MMEN = 12,
    MSPL = 13,
    MARMOR = 14,
    MTHAC = 15,
    MDMG = 16,
    MSPLDMG = 17,
    MABSORB = 18,
    MALIGN = 19,

    MAX_MOD = 20 // change when add more - up to 30
};

// Language Defines
enum Languages {
    LUNKNOWN = 0,
    LDWARVEN = 1,
    LELVEN = 2,
    LHALFLING = 3,
    LCOMMON = 4,
    LORCISH = 5,
    LGIANTKIN = 6,
    LGNOMISH = 7,
    LTROLL = 8,
    LOGRISH = 9,
    LDARKELVEN = 10,
    LGOBLINOID = 11,
    LMINOTAUR = 12,
    LCELESTIAL = 13,
    LKOBOLD = 14,
    LINFERNAL = 15,
    LBARBARIAN = 16,
    LKATARAN = 17,
    LDRUIDIC = 18,
    LWOLFEN = 19,
    LTHIEFCANT = 20,
    LARCANIC = 21,
    LABYSSAL = 22,
    LTIEFLING = 23,

    LANGUAGE_COUNT = 24
};


// Enums
enum LoadType {
    LS_NORMAL,
    LS_BACKUP,
    LS_CONVERT,

    LS_FULL,
    LS_PROTOTYPE,
    LS_REF
};


// positions in the color array
enum CustomColor {
    CUSTOM_COLOR_BROADCAST      = 0,

    CUSTOM_COLOR_GOSSIP     = 1,
    CUSTOM_COLOR_PTEST      = 2,
    CUSTOM_COLOR_NEWBIE     = 3,

    CUSTOM_COLOR_DM         = 4,
    CUSTOM_COLOR_ADMIN      = 5,
    CUSTOM_COLOR_SEND       = 6,
    CUSTOM_COLOR_MESSAGE        = 7,
    CUSTOM_COLOR_WATCHER        = 8,

    CUSTOM_COLOR_CLASS      = 9,
    CUSTOM_COLOR_RACE       = 10,
    CUSTOM_COLOR_CLAN       = 11,
    CUSTOM_COLOR_TELL       = 12,
    CUSTOM_COLOR_GROUP      = 13,
    CUSTOM_COLOR_DAMAGE     = 14,
    CUSTOM_COLOR_SELF       = 15,
    CUSTOM_COLOR_GUILD      = 16,
    MAX_CUSTOM_COLOR
};

enum WeatherString {
    WEATHER_SUNRISE,
    WEATHER_SUNSET,

    WEATHER_EARTH_TREMBLES,
    WEATHER_HEAVY_FOG,

    WEATHER_BEAUTIFUL_DAY,
    WEATHER_BRIGHT_SUN,
    WEATHER_GLARING_SUN,
    WEATHER_HEAT,

    WEATHER_STILL,
    WEATHER_LIGHT_BREEZE,
    WEATHER_STRONG_WIND,
    WEATHER_WIND_GUSTS,
    WEATHER_GALE_FORCE,

    WEATHER_CLEAR_SKIES,
    WEATHER_LIGHT_CLOUDS,
    WEATHER_THUNDERHEADS,

    WEATHER_LIGHT_RAIN,
    WEATHER_HEAVY_RAIN,
    WEATHER_SHEETS_RAIN,
    WEATHER_TORRENT_RAIN,

    WEATHER_NO_MOON,
    WEATHER_SLIVER_MOON,
    WEATHER_HALF_MOON,
    WEATHER_WAXING_MOON,
    WEATHER_FULL_MOON
};



// make sure these stay updated with calendar.xml!
enum Season {
    NO_SEASON = 0,
    SPRING =    1,
    SUMMER =    2,
    AUTUMN =    3,
    WINTER =    4
};

// for bit flags, take (season-1)^2
//   spring flag (1) = overflowing river
//   summer flag (2) = ?
//   autumn flag (4) = ?
//   winter flag (8) = cold damage, no herbs



// Religions
enum religions {
    ATHEIST     =   0,
    ARAMON      =   1,
    CERIS       =   2,
    ENOCH       =   3,
    GRADIUS     =   4,
    ARES        =   5,
    KAMIRA      =   6,
    LINOTHAN    =   7,
    ARACHNUS    =   8,
    MARA        =   9,
    JAKAR       =   10,
    MAX_DEITY
};

const int DEITY_COUNT = MARA+1;

enum class CreatureClass {
    NONE            =   0,
    ASSASSIN        =   1,
    BERSERKER       =   2,
    CLERIC          =   3,
    FIGHTER         =   4,
    MAGE            =   5,
    PALADIN         =   6,
    RANGER          =   7,
    THIEF           =   8,
    PUREBLOOD       =   9,
    MONK            =   10,
    DEATHKNIGHT     =   11,
    DRUID           =   12,
    LICH            =   13,
    WEREWOLF        =   14,
    BARD            =   15,
    ROGUE           =   16,
    BUILDER         =   17,
    CARETAKER       =   19,
    DUNGEONMASTER   =   20,
    CLASS_COUNT,
};

// start of the staff
const CreatureClass STAFF = (CreatureClass::BUILDER);


const int MULTI_BASE = static_cast<int>(CreatureClass::BUILDER);

const int CLASS_COUNT_MULT = (MULTI_BASE + 7);


// races
enum Races {
    UNKNOWN = 0,

    // playable races
    DWARF = 1,
    ELF = 2,
    HALFELF = 3,
    HALFLING = 4,
    HUMAN = 5,
    ORC = 6,
    HALFGIANT = 7,
    GNOME = 8,
    TROLL = 9,
    HALFORC = 10,
    OGRE = 11,
    DARKELF = 12,
    GOBLIN = 13,
    MINOTAUR = 14,
    SERAPH = 15,
    KOBOLD = 16,
    CAMBION = 17,
    BARBARIAN = 18,
    KATARAN = 19,
    TIEFLING = 20,

    MAX_PLAYABLE_RACE = 21,

    // non-playable
    LIZARDMAN = 21,
    CENTAUR = 22,

    // subraces, currently non-playable
    HALFFROSTGIANT = 23,
    HALFFIREGIANT = 24,
    GREYELF = 25,
    WILDELF = 26,
    AQUATICELF = 27,
    DUERGAR = 28,
    HILLDWARF = 29,

    // non-playable
    GNOLL = 30,
    BUGBEAR = 31,
    HOBGOBLIN = 32,
    WEMIC = 33,
    HYBSIL = 34,
    RAKSHASA = 35,
    BROWNIE = 36,
    FIRBOLG = 37,
    SATYR = 38,

    RACE_COUNT
};





// Actual value doesn't matter, just needs to be different than PLAYER & MONSTER
const int OBJECT = 2;
const int EXIT = 3;


// Proficiencies
//const int SHARP = 0;
//const int THRUST = 1;
//const int BLUNT = 2;
//const int POLE = 3;
//const int MISSILE = 4;
//const int CLEAVE = 5;

enum class ObjectType {
    // 0,1,2 open

    INSTRUMENT = 3, // Bard Instrument

    HERB = 4,   // Item is a herb!  Subtype if needed in subtype

    WEAPON = 5,

    ARMOR = 6,
    POTION = 7,
    SCROLL = 8,
    WAND = 9,
    CONTAINER = 10,

    MONEY = 11,
    KEY = 12,
    LIGHTSOURCE = 13,
    MISC = 14,
    SONGSCROLL = 15,
    POISON = 16,
    BANDAGE = 17,
    AMMO = 18,
    QUIVER = 19,
    LOTTERYTICKET = 20,

    MAX_OBJECT_TYPE,
};


enum class CastType {
    POTION,
    SCROLL,
    WAND,
    CAST,
    SKILL,
};

// Spell Realms

const int CONJUREMAGE = 7;
const int CONJUREBARD = 8;
const int CONJUREANIM = 9;




// Maximum number of items that can be worn/readied
const int MAXWEAR = 20;

// Wear locations
enum WearLocations {
    BODY = 1,
    ARMS = 2,
    LEGS = 3,
    NECK = 4,
    BELT = 5,
    HANDS = 6,
    HEAD = 7,
    FEET = 8,
    FINGER = 9,
    FINGER1 = 9,
    FINGER2 = 10,
    FINGER3 = 11,
    FINGER4 = 12,
    FINGER5 = 13,
    FINGER6 = 14,
    FINGER7 = 15,
    FINGER8 = 16,
    HELD = 17,
    SHIELD = 18,
    FACE = 19,
    WIELD = 20
};


enum DeathType {
    DT_NONE,
    // Death types
    FALL,
    POISON_MONSTER,
    POISON_GENERAL,
    DISEASE,
    SMOTHER,
    FROZE,
    BURNED,
    DROWNED,
    DRAINED,
    ZAPPED,
    SHOCKED,
    WOUNDED,
    CREEPING_DOOM,
    SUNLIGHT,

    // Trap death types
    PIT,
    BLOCK,
    DART,
    ARROW,
    SPIKED_PIT,
    FIRE_TRAP,
    FROST,
    ELECTRICITY,
    ACID,
    ROCKS,
    ICICLE_TRAP,
    SPEAR,
    CROSSBOW_TRAP,
    VINES,
    COLDWATER,
    EXPLODED,
    BOLTS,
    SPLAT,
    POISON_PLAYER,
    BONES,
    EXPLOSION,
    PETRIFIED,
    LIGHTNING,
    WINDBATTERED,
    PIERCER,
    ELVEN_ARCHERS,
    DEADLY_MOSS,
    THORNS
};


// Weather
const int WSUNNY = 1;   // Sunny outside
const int WWINDY = 2;   // Rainy outside
const int WSTORM = 3;   // Storm
const int WMOONF = 4;   // Full Moon

// specials
const int SP_MAPSC = 1;     // Map or scroll
const int SP_COMBO = 2;     // Combination lock
const int MAX_SP = 3;

enum Bits {
    BIT0 = (1<<0),
    BIT1 = (1<<1),
    BIT2 = (1<<2),
    BIT3 = (1<<3),
    BIT4 = (1<<4),
    BIT5 = (1<<5),
    BIT6 = (1<<6),
    BIT7 = (1<<7),
    BIT8 = (1<<8),
    BIT9 = (1<<9),
    BIT10 = (1<<10),
    BIT11 = (1<<11),
    BIT12 = (1<<12)
};

// findTarget search places
enum FindTargets {
    FIND_OBJ_EQUIPMENT = BIT0,
    FIND_OBJ_INVENTORY = BIT1,
    FIND_OBJ_ROOM = BIT2,
    FIND_MON_ROOM = BIT3,
    FIND_PLY_ROOM = BIT4,
    FIND_EXIT = BIT5
};

// obj_str and crt_str flags
enum StrFlags {
    CAP = BIT0,
    INV = BIT1,
    MAG = BIT2,
    ISDM = BIT3,
    MIST = BIT4,
    ISBD = BIT5,
    ISCT = BIT6,
    NONUM = BIT7
};

enum Color {
    NOCOLOR     = 0,
    BLACK       = BIT0,
    RED         = BIT1,
    GREEN       = BIT2,
    YELLOW      = BIT3,
    BLUE        = BIT4,
    MAGENTA     = BIT5,
    CYAN        = BIT6,
    WHITE       = BIT7,
    BOLD        = BIT8,
    NORMAL      = BIT9,
    BLINK       = BIT10,
    UNDERLINE   = BIT11,
    MAX_COLOR
};

enum AttackResultFlags {
    NO_FLAG     = 0,
    NO_DODGE    = BIT0,
    NO_PARRY    = BIT1,
    NO_BLOCK    = BIT2,
    NO_CRITICAL = BIT3,
    NO_FUMBLE   = BIT4,
    NO_GLANCING = BIT5,
    DOUBLE_MISS = BIT6,
    USE_LEVEL   = BIT7,

    MAX_ATTACK_FLAG
};


const int COLOR_BLACK = 0;
const int COLOR_RED = 1;
const int COLOR_GREEN = 2;
const int COLOR_YELLOW = 3;
const int COLOR_BLUE = 4;
const int COLOR_MAGENTA = 5;
const int COLOR_CYAN = 6;
const int COLOR_WHITE = 7;
const int COLOR_BOLD = 8;
const int COLOR_NORMAL = 9;
const int COLOR_BLINK = 10;

//#define BOLD      8
//#define NORMAL    9
//#define BLINK     10
//#define UNDERLINE 11



// Enums for effects
// Where is this effect being applied from?
enum ApplyFrom {
    FROM_NOWHERE,

    FROM_CREATURE,
    FROM_MONSTER,
    FROM_PLAYER,
    FROM_OBJECT,
    FROM_POTION,
    FROM_WAND,
    FROM_ROOM,

    MAX_FROM
};

// Actions to be taken by the effect function
enum EffectAction {
    NO_ACTION,

    EFFECT_COMPUTE, // Compute strength, duration, etc -- DO THIS BEFORE ADDING!!!!
    EFFECT_APPLY,   // Apply any bonuses
    EFFECT_UNAPPLY, // Unapply any bonuses
    EFFECT_PULSE,

    MAX_ACTION
};


enum Realm {
    NO_REALM =  0,
    MIN_REALM = 1,
    EARTH =     1,
    WIND =      2,
    FIRE =      3,
    WATER =     4,
    ELEC =      5,
    COLD =      6,

    MAX_REALM
};


enum PropType {
    PROP_NONE = 0,
    PROP_STORAGE = 1,
    PROP_SHOP = 2,
    PROP_GUILDHALL = 3,
    PROP_HOUSE = 4,

    PROP_END = 5
};

enum UnequipAction {
    UNEQUIP_ADD_TO_INVENTORY,
    UNEQUIP_DELETE,
    UNEQUIP_NOTHING
};


enum CastResult {
    CAST_RESULT_FAILURE,            // failure, unlikely it will succeed
    CAST_RESULT_CURRENT_FAILURE,    // failure, it may succeed in the future
    CAST_RESULT_SPELL_FAILURE,      // failure, the spell didn't work
    CAST_RESULT_SUCCESS
};


#endif /* GLOBAL_H_ */

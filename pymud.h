/*
 * mud.h
 *	 Defines required by the rest of the program.
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
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef MUD_H
#define MUD_H

// Other includes are at the end of the file to make sure all defines and such in this
// file are visible

#ifndef PYTHON_CODE_GEN
#include <Python.h> // Python!
#endif

#include "bstring.h"

#ifndef READCFG
#define READCFG
#endif // READCFG

#ifndef FALSE
#define FALSE		(0)
#endif

#ifndef TRUE
#define TRUE	 	(!FALSE)
#endif

// Size of exp array, also highest you can train
#define MAXALVL 	40
#define MAX_ARMOR	2000
// I/O buffer sizes
#define IBUFSIZE	1024
#define OBUFSIZE	8192

// File permissions
//#define S_IWRITE	00660
//#define S_IREAD	00006

#ifndef O_BINARY
#define O_BINARY	0
#endif
#define ACC		00666

// merror() error types
#define FATAL		1
#define NONFATAL	0

enum LoadType {
	LS_NORMAL,
	LS_BACKUP,
	LS_CONVERT,

	LS_FULL,
	LS_REF
};

#define CUSTOM_COLOR_DEFAULT		'#'
#define CUSTOM_COLOR_SIZE		20

// positions in the color array
enum CustomColor {
	CUSTOM_COLOR_BROADCAST		= 0,

	CUSTOM_COLOR_GOSSIP		= 1,
	CUSTOM_COLOR_PTEST		= 2,
	CUSTOM_COLOR_NEWBIE		= 3,

	CUSTOM_COLOR_DM			= 4,
	CUSTOM_COLOR_ADMIN		= 5,
	CUSTOM_COLOR_SEND		= 6,
	CUSTOM_COLOR_MESSAGE		= 7,
	CUSTOM_COLOR_WATCHER		= 8,

	CUSTOM_COLOR_CLASS		= 9,
	CUSTOM_COLOR_RACE		= 10,
	CUSTOM_COLOR_CLAN		= 11,
	CUSTOM_COLOR_TELL		= 12,
	CUSTOM_COLOR_GROUP		= 13,
	CUSTOM_COLOR_DAMAGE		= 14,
	CUSTOM_COLOR_SELF		= 15,
	CUSTOM_COLOR_GUILD		= 16,
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


// base monster number for various types of summoned monsters
#define BASE_ELEMENTAL	801
#define BASE_UNDEAD	550
#define SLOWTICK

#define COMMANDMAX	6

// Monster and object files sizes (in terms of monsters or objects)
#define MFILESIZE	100
#define OFILESIZE	100

// memory limits

#define RMAX		20000	// Max number of these allowed to be created
#define MMAX		20000
#define OMAX		20000
#define PMAX		1024

#define RQMAX		600	 // Max number of these allowed in memory
#define MQMAX		200	 // at any one time
#define OQMAX		200


// make sure these stay updated with calendar.xml!
enum Season {
	NO_SEASON =	0,
	SPRING =	1,
	SUMMER =	2,
	AUTUMN =	3,
	WINTER =	4
};

// for bit flags, take (season-1)^2
//   spring flag (1) = overflowing river
//   summer flag (2) = ?
//   autumn flag (4) = ?
//   winter flag (8) = cold damage, no herbs


#define BROADLOG		1

#define MAX_MULTICLASS		8
// how often (in seconds) players get saved
#define SAVEINTERVAL		1200

#define MAXTIMEOUT		14400

#define MAXPAWN			25000

#define MAX_QUEST		128

#define MAX_FACTION		32
#define MAX_DUELS		10
#define MAX_BUILDER_RANGE	10

// Save flags
#define PERMONLY	1
#define ALLITEMS	0

// Command status returns
#define DISCONNECT	1
#define PROMPT		2
#define DOPROMPT	3



#define WIELDOBJ	1
#define SECONDOBJ	2
#define HOLDOBJ		3
#define SHIELDOBJ	4



// NPC Trades

#define NOTRADE		0
#define SMITHY		1
#define BANKER		2
#define ARMOROR		3
#define WEAPONSMITH	4
#define MERCHANT	5
#define TRAINING_PERM	6



#define MOBTRADE_COUNT	7

// Monetary transaction types
#define BUY		0
#define SELL		1

// Creature stats
#define STR		1
#define DEX		2
#define CON		3
#define INT		4
#define PTY		5
#define CHA		6
#define MAX_STAT	PTY

#define MIN_STAT_NUM	10
#define MAX_STAT_NUM	MAXALVL*10

// Saving throw types
#define LCK			0  // Luck
#define POI			1  // Poison
#define DEA			2  // Death
#define BRE			3  // Breath Weapons
#define MEN			4  // Mental
#define SPL			5  // Spells

#define MAX_SAVE		6
#define MAX_SAVE_COLOR		11

// Religions
enum religions {
ATHEIST		=	0,
ARAMON		=	1,
CERIS		=	2,
ENOCH		=	3,
GRADIUS		=	4,
ARES		=	5,
KAMIRA		=	6,
LINOTHAN	=	7,
ARACHNUS	=	8,
MARA		=	9,
JAKAR		=	10,
MAX_DEITY
};
#define DEITY_COUNT		MARA+1

//#define ATHEIST		0
//#define ARAMON		1
//#define CERIS			2
//#define ENOCH			3
//#define GRADIUS		4
//#define ARES			5
//#define KAMIRA		6
//#define LINOTHAN		7
//#define ARACHNUS		8
//#define MARA			9
//#define JAKAR			10
//
//#define DEITY_COUNT		MARA+1

// Alignments
#define BLOODRED	-3
#define REDDISH		-2
#define PINKISH		-1
#define NEUTRAL		0
#define LIGHTBLUE	1
#define BLUISH		2
#define ROYALBLUE	3

// Attack types
#define PHYSICAL		1
#define MAGICAL			2
#define MENTAL			3
#define NEGATIVE_ENERGY		4
#define ABILITY			5
#define MAGICAL_NEGATIVE	6

// Modifier List
#define MSTR		0
#define MDEX		1
#define MCON		2
#define MINT		3
#define MPIE		4
#define MCHA		5
#define MHP		6
#define MMP		7
#define MLCK		8
#define MPOI		9
#define MDEA		10
#define MBRE		11
#define MMEN		12
#define MSPL		13
#define MARMOR		14
#define MTHAC		15
#define MDMG		16
#define MSPLDMG		17
#define MABSORB		18
#define MALIGN		19

#define MAX_MOD		20 // change when add more - up to 30

// Language Defines

#define LUNKNOWN	0
#define LDWARVEN	1
#define LELVEN	  	2
#define LHALFLING   	3
#define LCOMMON	 	4
#define LORCISH	 	5
#define LGIANTKIN   	6
#define LGNOMISH	7
#define LTROLL	  	8
#define LOGRISH	 	9
#define LDARKELVEN  	10
#define LGOBLINOID  	11
#define LMINOTAUR   	12
#define LCELESTIAL  	13
#define LKOBOLD	 	14
#define LINFERNAL   	15
#define LBARBARIAN	16
#define LKATARAN	17
#define LDRUIDIC	18
#define LWOLFEN	 	19
#define LTHIEFCANT  	20
#define LARCANIC	21
#define LABYSSAL	22
#define LTIEFLING	23

#define LANGUAGE_COUNT	24


//// Object Material Types
//#define CLOTH		0
//#define GLASS		1
//#define LEATHER	2
//#define IRON		3
//#define PAPER		4
//#define STONE		5
//#define WOOD		6
//#define LIQUID	7
//#define VOLATILE	8
//#define BONE		9
//#define GEMS		10
//#define DIAMOND	11
//#define JEWELS	12
//#define STEEL		13
//#define MALLEABLE	14
//#define MITHRIL	15
//#define ADAMANTIUM	16
//#define SCALE		17
//#define DRAGONSCALE	18

enum crtClasses {
ASSASSIN	=	1,
BERSERKER	=	2,
CLERIC		=	3,
FIGHTER		=	4,
MAGE		=	5,
PALADIN		=	6,
RANGER		=	7,
THIEF		=	8,
VAMPIRE		=	9,
MONK		=	10,
DEATHKNIGHT	=	11,
DRUID		=	12,
LICH		=	13,
WEREWOLF	=	14,
BARD		=	15,
ROGUE		=	16,
BUILDER		=	17,
CARETAKER	=	19,
DUNGEONMASTER	=	20,
CLASS_COUNT
};

// Character classes
//#define ASSASSIN		1
//#define BERSERKER		2
//#define CLERIC		3
//#define FIGHTER		4
//#define MAGE			5
//#define PALADIN		6
//#define RANGER		7
//#define THIEF			8
//#define VAMPIRE		9
//#define MONK			10
//#define DEATHKNIGHT		11
//#define DRUID			12
//#define LICH			13
//#define WEREWOLF		14
//#define BARD			15
//#define ROGUE			16
//#define BUILDER		17
////#define CREATOR		18
//#define CARETAKER		19
//#define DUNGEONMASTER		20

// start of the staff
#define STAFF			(BUILDER)

#define MULTI_BASE		(BUILDER)
//#define CLASS_COUNT		(DUNGEONMASTER + 1)
#define CLASS_COUNT_MULT	(MULTI_BASE + 7)


// races
#define UNKNOWN			0

// playable races
#define DWARF	  		1
#define ELF			2
#define HALFELF			3
#define HALFLING	 	4
#define HUMAN	  		5
#define ORC			6
#define HALFGIANT		7
#define GNOME			8
#define TROLL			9
#define HALFORC			10
#define OGRE			11
#define DARKELF			12
#define GOBLIN			13
#define MINOTAUR		14
#define SERAPH			15
#define KOBOLD			16
#define CAMBION			17
#define BARBARIAN		18
#define KATARAN			19
#define TIEFLING		20


#define MAX_PLAYABLE_RACE	TIEFLING + 1

// non-playable
#define LIZARDMAN		21
#define CENTAUR			22

// subraces, currently non-playable
#define HALFFROSTGIANT		23
#define HALFFIREGIANT		24
#define GREYELF			25
#define WILDELF			26
#define AQUATICELF		27
#define DUERGAR			28
#define HILLDWARF		29

// non-playable
#define GNOLL			30
#define BUGBEAR			31
#define HOBGOBLIN		32
#define WEMIC			33
#define HYBSIL			34
#define RAKSHASA		35
#define BROWNIE			36
#define FIRBOLG			37
#define SATYR			38

#define RACE_COUNT		SATYR + 1

// Actual value doesn't matter, just needs to be different than PLAYER & MONSTER
#define OBJECT			2
#define EXIT			3


// Proficiencies
//#define SHARP			0
//#define THRUST		1
//#define BLUNT			2
//#define POLE			3
//#define MISSILE		4
//#define CLEAVE		5

// Bard Instrument
#define INSTRUMENT      3
// Item is a herb!  Subtype if needed in subtype
#define HERB			4

#define WEAPON			5
// object types
#define ARMOR			6
#define POTION			7
#define SCROLL			8
#define WAND			9
#define CONTAINER		10

#define MONEY			11
#define KEY			    12
#define LIGHTSOURCE		13
#define MISC			14
#define SONGSCROLL		15
#define POISON			16
#define BANDAGE			17
#define AMMO			18
#define QUIVER			19
#define LOTTERYTICKET	20

#define MAX_OBJ_TYPE	21

#define DEFAULT_WEAPON_DELAY	30 // 3 seconds

// Spell Realms


#define CONJUREMAGE		7
#define CONJUREBARD		8
#define CONJUREANIM		9

// Daily use variables
#define DL_BROAD		0	// Daily broadcasts
#define DL_ENCHA		1	// Daily enchants
#define DL_FHEAL		2	// Daily heals
#define DL_TRACK		3	// Daily track spells
#define DL_DEFEC		4	// Daily defecations
#define DL_RCHRG		5	// Daily wand recharges
#define DL_TELEP		6	// Daily tports
#define DL_BROAE		7	// Daily broademotes
#define DL_ETRVL		8	// Daily ethereal travels
#define DL_HANDS		9	// Daily Lay on Hands
#define DL_RESURRECT		10	// Daily resurrections

#define DL_SCARES		11	// Daily scare casts
#define DL_SILENCE		12
#define DL_HARM			13	// Daily harm casts


#define DAILYLAST		13

// Object Last-time stuff
#define LT_ENCHA		0
#define LT_ENVEN		1
#define DONT_USE		2

// Last-time specifications
#define OLD_LT_INVISIBILITY		0
#define OLD_LT_PROTECTION		1
#define OLD_LT_BLESS			2
#define OLD_LT_INFRAVISION		13
#define OLD_LT_DETECT_INVIS		17
#define OLD_LT_DETECT_MAGIC		18
#define OLD_LT_LEVITATE			21
#define OLD_LT_HEAT_PROTECTION		23
#define OLD_LT_FLY			24
#define OLD_LT_RESIST_MAGIC		25
#define OLD_LT_KNOW_AURA		27
#define OLD_LT_RESIST_COLD		29
#define OLD_LT_BREATHE_WATER		30
#define OLD_LT_EARTH_SHIELD		31
#define OLD_LT_RESIST_WATER		52
#define OLD_LT_RESIST_FIRE		53
#define OLD_LT_RESIST_AIR		54
#define OLD_LT_RESIST_EARTH		55
#define OLD_LT_REFLECT_MAGIC		56
#define OLD_LT_TRUE_SIGHT		60
#define OLD_LT_CAMOUFLAGE		61
#define OLD_LT_DRAIN_SHIELD		63
#define OLD_LT_UNDEAD_WARD   		65
#define OLD_LT_RESIST_ELEC		66
#define OLD_LT_WARMTH			67
#define OLD_LT_HASTE			71
#define OLD_LT_SLOW			72
#define OLD_LT_STRENGTH			78
#define OLD_LT_ENFEEBLEMENT		79
#define OLD_LT_DARKNESS			88
#define OLD_LT_TONGUES			90
#define OLD_LT_INSIGHT			93
#define OLD_LT_FEEBLEMIND		94
#define OLD_LT_PRAYER			95
#define OLD_LT_DAMNATION		96
#define OLD_LT_FORTITUDE		97
#define OLD_LT_WEAKNESS			98
#define OLD_LT_PASS_WITHOUT_TRACE	99
#define OLD_LT_COMPREHEND_LANGUAGES	74
#define OLD_LT_FARSIGHT			102
#define OLD_LT_HOLD_PERSON		75
#define OLD_LT_CONFUSED			80
#define OLD_LT_TEMP_BLIND		45  // Was LT_CHARM which wasn't being used.
#define OLD_LT_SILENCE			34
#define OLD_LT_ARMOR			39
#define OLD_LT_ATTACK			3


// free				0
// free				1
// free				2
#define LT_AGGRO_ACTION		3
#define LT_TRACK		4
#define LT_MON_SCAVANGE		4
#define LT_STEAL		5
#define LT_PICKLOCK		6
#define LT_MON_WANDER		6
#define LT_SEARCH		7
#define LT_TICK			8	// Hp/Poison ticking timer
#define LT_SPELL		9
#define LT_PEEK			10
#define LT_SNEAK		11
#define LT_READ_SCROLL		12
// free				13
#define LT_HIDE			14
#define LT_TURN			15
#define LT_FRENZY		16
// free				17
// free				18
#define LT_PRAY			19
#define LT_PREPARE		20
// free				21
#define LT_PLAYER_SAVE		22
// free				23
// free				24
// free				25
#define LT_MOVED		26
// free				27
#define LT_AGE			28
// free				29
// free				30
// free				31
#define LT_SONG_PLAYED		33
#define LT_SING			35
#define LT_CHARMED		36
#define LT_MEDITATE		37
#define LT_TOUCH_OF_DEATH	38
// free				38
#define LT_BLOOD_SACRIFICE	40
#define LT_MOB_THIEF		41
#define LT_INVOKE		42
#define LT_SCOUT		43
#define LT_BERSERK  	  	44

#define LT_REGENERATE		46
#define LT_DRAIN_LIFE		47
#define LT_HYPNOTIZE		48
#define LT_PLAYER_BITE		49
#define LT_FREE_ACTION		50
#define LT_PLAYER_SEND		51
// free				52
// free				53
// free				54
// free				55
// free				56
#define LT_IDENTIFY		57
#define LT_FOCUS		58
#define LT_PLAYER_STUNNED	59
// free				60
// free				61
#define LT_RP_AWARDED		62
// free				63
#define LT_UNCONSCIOUS		64
// free				65
// free				66
// free				67
// free				68
#define LT_MOB_JAILED		69
#define LT_LAY_HANDS		70
// free				71
// free				72
#define LT_NO_PKILL		73
#define LT_MOB_RENOUNCE		74
#define LT_SAVES		76

#define LT_NO_BROADCAST		77
// free				78
// free				79
// free				80
#define LT_NOMPTICK		81
#define LT_NOHPTICK		82
// free				83
#define LT_KILL_DOCTOR		84
// free				85
#define LT_LEVEL_DRAIN		86
#define LT_HOWLS 		87
// free				88
#define LT_SKILL_INCREASE	89
// free				90
#define LT_ENLARGE_REDUCE	91
#define LT_ANCHOR		92
// free				93
// free				94
// free				95
// free				96
// free				97
// free				98
// free				99
#define LT_TICK_SECONDARY	100	// Mp/Focus ticking
#define LT_TICK_HARMFUL		101	// Poison/Pharm etc

#define MAX_LT	  		103	 // Incriment when you add any new LTs.


#define LT_JAILED  		127	// Higher than max LT so it "ticks" when player is offline.
#define LT_MOBDEATH		126	// Ticks down offline

// Max possible LT is 126 currently. LT 127 is used for *jail - TC
#define TOTAL_LTS		127



//#define LT_MOB_PASV_GUARD	LT_UNCONSCIOUS	// Mobs use LT_UNCONSCIOUS cuz they don't need it anywhere else
#define LT_RENOUNCE		LT_TOUCH_OF_DEATH
#define LT_HOLYWORD		LT_FOCUS
#define LT_SMOTHER		LT_FOCUS

// BUG: Animate & tail slap both use LT_FOCUS -> high level animate tail slaps
#define LT_ANIMATE 		LT_FOCUS

#define LT_MOB_BASH		LT_IDENTIFY
#define LT_RIPOSTE		LT_MON_SCAVANGE

#define LT_DISARM		LT_FOCUS
#define LT_TAIL_SLAP		LT_FOCUS   // Mobs use LT_FOCUS on tail slap since they don't ever use focus.
#define LT_DEATH_SCREAM		LT_BERSERK   // Mobs use LT_BERSERK for death scream since they don't ever berserk.
#define LT_OUTLAW		LT_MOB_THIEF
#define LT_KICK			LT_IDENTIFY   // No one that kicks also identifies.
#define LT_MAUL			LT_IDENTIFY   // No one that mauls also identifies
#define LT_MIST			LT_MOB_THIEF
#define LT_BARKSKIN		LT_FOCUS
#define LT_PETRIFYING_GAZE 	LT_MOB_JAILED

#define LT_MISTBANE		LT_FOCUS
#define LT_MOB_BREATH		LT_HYPNOTIZE
#define LT_MOB_TRAMPLE		LT_ARMOR

#define LT_ENDURANCE		LT_FREE_ACTION
#define LT_M_AURA_ATTACK	LT_NO_PKILL

#define OLD_LT_STONESKIN	LT_MON_WANDER


// Maximum number of items that can be worn/readied
#define MAXWEAR		20

// Wear locations
#define BODY		1
#define ARMS		2
#define LEGS		3
#define NECK		4
#define BELT		5
#define HANDS		6
#define HEAD		7
#define FEET		8
#define FINGER		9
#define FINGER1		9
#define FINGER2		10
#define FINGER3		11
#define FINGER4		12
#define FINGER5		13
#define FINGER6		14
#define FINGER7		15
#define FINGER8		16
#define HELD		17
#define SHIELD		18
#define FACE		19
#define WIELD		20


// Song flags
#define SONG_HEAL		0	// Healing
#define SONG_MP			1	// Magic-Restoration
#define SONG_RESTORE		2	// Restoration
#define SONG_DESTRUCTION	3	// Destruction
#define SONG_MASS_DESTRUCTION	4	// Mass Destruction
#define SONG_BLESS		5	// Room-Bless
#define SONG_PROTECTION		6	// Room-Protection
#define SONG_FLIGHT		7	// Song of Flight
#define SONG_RECALL		8	// Song of Recall
#define SONG_SAFETY		9	// Song of Safety


//#define MAXSONG		10

#define MAX_AURAS		6	// Max mob aura attacks

// Spell casting types
#define CAST			0
#define SKILL			1	// Druid summon-elemental

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

#include "flags.h"

// Weather
#define WSUNNY		1	// Sunny outside
#define WWINDY		2	// Rainy outside
#define WSTORM		3	// Storm
#define WMOONF		4	// Full Moon

// specials
#define SP_MAPSC	1		// Map or scroll
#define SP_COMBO	2		// Combination lock
#define MAX_SP		3


#define RETURN(a,b,c)   Ply[a].io->fn = b; Ply[a].io->fnparam = c; return

#define BOOL(a)		((a) ? 1 : 0)

// Minimum and maximum values a `signed int' can hold.
#ifndef __INT_MAX__
#define __INT_MAX__ 2147483647
#endif
#undef INT_MIN
#define INT_MIN (-INT_MAX-1)
#undef INT_MAX
#define INT_MAX __INT_MAX__

// Maximum value an `unsigned int' can hold.  (Minimum is 0).
#undef UINT_MAX
#define UINT_MAX (INT_MAX * 2U + 1)

#ifndef MAXINT
#define MAXINT INT_MAX
#endif	//MAXINT
// if ltime is 0, assume uninitialized and return MAXINT.  This is so monsters
//   can have spell flags set and have them be permanent.
#define LT(a,b)		((a)->lasttime[(b)].ltime ? (a)->lasttime[(b)].ltime + (a)->lasttime[(b)].interval : MAXINT)

//#define WISDOM(a)		((bonus((int)(a)->intelligence.getCur())+bonus[(int)(a)->piety.getCur()])/2)
//#define AWARENESS(a)	((bonus((int)(a)->intelligence.getCur())+bonus[(int)(a)->dexterity.getCur()])/2)

#define mrand(a,b)	((a)+(rand()%((b)*10-(a)*10+10))/10)
#define MIN(a,b)	(((a)<(b)) ? (a):(b))
#define MAX(a,b)	(((a)>(b)) ? (a):(b))

#define PLYCRT(p)	((p)->isPlayer() ? "Player" : "Monster")

#define AC(p)		((int)((p)->getArmor()) / 10)

#define BIT0		(1<<0)
#define BIT1		(1<<1)
#define BIT2		(1<<2)
#define BIT3		(1<<3)
#define BIT4		(1<<4)
#define BIT5		(1<<5)
#define BIT6		(1<<6)
#define BIT7		(1<<7)
#define BIT8		(1<<8)
#define BIT9		(1<<9)
#define BIT10		(1<<10)
#define BIT11		(1<<11)
#define BIT12		(1<<12)

enum Color {
	NOCOLOR		= 0,
	BLACK 		= BIT0,
	RED 		= BIT1,
	GREEN		= BIT2,
	YELLOW		= BIT3,
	BLUE		= BIT4,
	MAGENTA		= BIT5,
	CYAN		= BIT6,
	WHITE		= BIT7,
	BOLD		= BIT8,
	NORMAL		= BIT9,
	BLINK		= BIT10,
	UNDERLINE	= BIT11,
	MAX_COLOR
};

enum AttackResultFlags {
	NO_FLAG		= 0,
	NO_DODGE	= BIT0,
	NO_PARRY	= BIT1,
	NO_BLOCK	= BIT2,
	NO_CRITICAL	= BIT3,
	NO_FUMBLE	= BIT4,
	NO_GLANCING	= BIT5,
	DOUBLE_MISS	= BIT6,
	USE_LEVEL	= BIT7,

	MAX_ATTACK_FLAG
};
// findTarget search places
#define FIND_OBJ_EQUIPMENT	BIT0
#define FIND_OBJ_INVENTORY	BIT1
#define FIND_OBJ_ROOM		BIT2
#define FIND_MON_ROOM		BIT3
#define FIND_PLY_ROOM		BIT4
#define FIND_EXIT			BIT5

// obj_str and crt_str flags
#define CAP		BIT0
#define INV		BIT1
#define MAG		BIT2
#define ISDM		BIT3
#define MIST		BIT4
#define ISBD		BIT5
#define ISCT		BIT6
#define NONUM		BIT7
#define USEANSI		BIT8
#define USEMIRC		BIT9

#define COLOR_BLACK	0
#define COLOR_RED	1
#define COLOR_GREEN	2
#define COLOR_YELLOW	3
#define COLOR_BLUE	4
#define COLOR_MAGENTA	5
#define COLOR_CYAN	6
#define COLOR_WHITE	7
#define COLOR_BOLD	8
#define COLOR_NORMAL	9
#define COLOR_BLINK	10

//#define BOLD		8
//#define NORMAL	9
//#define BLINK		10
//#define UNDERLINE	11



//*********************************************************************
//	Below this line are customizable options for the mud
//*********************************************************************

#define MINSHOPLEVEL		16
#define SURNAME_LEVEL		13
#define ALIGNMENT_LEVEL		6


#define SUNRISE			6
#define SUNSET			20


#define ROOM_BOUND_FAILURE	2	// room to go to if getBound fails

/*			Web Editor
 * 			 _   _  ____ _______ ______
 *			| \ | |/ __ \__   __|  ____|
 *			|  \| | |  | | | |  | |__
 *			| . ` | |  | | | |  |  __|
 *			| |\  | |__| | | |  | |____
 *			|_| \_|\____/  |_|  |______|
 *
 * 		If you change anything here, make sure the changes are reflected in the web
 * 		editor! Either edit the PHP yourself or tell Dominus to make the changes.
 */


#define MAX_DIMEN_ANCHORS	3

// toll to charge in case none is set on exit
#define DEFAULT_TOLL		100

#define MAX_MOBS_IN_ROOM	200

// For various effects that create objects
#define SHIT_OBJ		349
#define CORPSE_OBJ		800
#define BODYPART_OBJ		21
#define STATUE_OBJ		39
#define MONEY_OBJ		0
#define TICKET_OBJ		200

// When a player forces a pet to cast, this is the delay.
#define PET_CAST_DELAY		4


//
//// If a player dies X times in X hours, they don't lose exp for that death.
//// LUCKY_DEATHS should be defined as x - 1: if they get a free restore after
//// 3 deaths in 24 hours, enter 2. Set LUCKY_DEATH_HOURS to 0 to disable
//// lucky dying.
//#define LUCKY_DEATHS		2
//#define LUCKY_DEATH_HOURS	24

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
	EFFECT_APPLY,	// Apply any bonuses
	EFFECT_UNAPPLY,	// Unapply any bonuses
	EFFECT_PULSE,

	MAX_ACTION
};


enum Realm {
	NO_REALM =	0,
	MIN_REALM =	1,
	EARTH =		1,
	WIND =		2,
	FIRE =		3,
	WATER = 	4,
	ELEC =		5,
	COLD =		6,

	MAX_REALM
};


enum PropType {
	PROP_NONE =		0,
	PROP_STORAGE =		1,
	PROP_SHOP =		2,
	PROP_GUILDHALL =	3,
	PROP_HOUSE =		4,

	PROP_END =		5
};

enum UnequipAction {
	UNEQUIP_ADD_TO_INVENTORY,
	UNEQUIP_DELETE,
	UNEQUIP_NOTHING
};


enum CastResult {
	CAST_RESULT_FAILURE,			// failure, unlikely it will succeed
	CAST_RESULT_CURRENT_FAILURE,	// failure, it may succeed in the future
	CAST_RESULT_SPELL_FAILURE,		// failure, the spell didn't work
	CAST_RESULT_SUCCESS
};

// C includes
#ifndef PYTHON_CODE_GEN
	#include <fcntl.h>
#endif

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

// C++ Includes
#include <list>

// Mud Includes
#include "os.h"

#include "catRef.h"
#include "swap.h"
#include "structs.h"
#include "range.h"
#include "carry.h"
#include "container.h"
#include "xml.h"

#include "size.h"
#include "socket.h"
#include "stats.h"
#include "rooms.h"
#include "objects.h"

#include "skills.h"
#include "magic.h"
#include "fighters.h"
#include "creatures.h"

//#include "bans.h"
//#include "guilds.h"
//#include "factions.h"

#include "catRefInfo.h"
#include "startlocs.h"
#include "raceData.h"
#include "deityData.h"

#include "playerTitle.h"
#include "skillGain.h"
#include "levelGain.h"
#include "playerClass.h"
#include "fishing.h"

#include "server.h"
#include "config.h"

#include "mextern.h"

#include "paths.h"



#ifdef PYTHON_CODE_GEN
// Generate effects bindings as well
#include <effects.h>
#include <songs.h>
void broadcast(Socket* ignore, BaseRoom* room, const char *fmt, ...);
void broadcast(Socket* ignore1, Socket* ignore2, BaseRoom* room, const char *fmt, ...);
void broadcast(bool showTo(Socket*), Socket*, BaseRoom* room, const char *fmt, ...);


//void doBroadCast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt, va_list ap, Creature* player = NULL);
void broadcast(const char *fmt, ...);
void broadcast(int color, const char *fmt,...);
void broadcast(bool showTo(Socket*), bool showAlso(Socket*), const char *fmt,...);
void broadcast(bool showTo(Socket*), const char *fmt,...);
void broadcast(bool showTo(Socket*), int color, const char *fmt,...);
void broadcast(Creature* player, bool showTo(Socket*), int color, const char *fmt,...);

void broadcast_wc(int color,const char *fmt, ...);
void broadcast_login(Player* player, int login);

void broadcast_rom_LangWc(int face, int color, int lang, Socket* ignore, AreaRoom* aRoom, CatRef cr, const char *fmt,...);
void broadcastGroup(bool dropLoot, Creature* player, const char *fmt, ...);

#endif

#endif

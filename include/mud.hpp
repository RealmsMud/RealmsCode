/*
 * mud.h
 *   Defines required by the rest of the program.
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

#pragma once

class LastTime;

// Other includes are at the end of the file to make sure all defines and such in this
// file are visible

#ifndef READCFG
#define READCFG
#endif // READCFG

#ifndef FALSE
#define FALSE       (0)
#endif

#ifndef TRUE
#define TRUE        (!FALSE)
#endif

#ifndef O_BINARY
#define O_BINARY    0
#endif
#define ACC     00666




// Daily use variables
#define DL_BROAD        0   // Daily broadcasts
#define DL_ENCHA        1   // Daily enchants
#define DL_FHEAL        2   // Daily heals
#define DL_TRACK        3   // Daily track spells
#define DL_DEFEC        4   // Daily defecations
#define DL_RCHRG        5   // Daily wand recharges
#define DL_TELEP        6   // Daily tports
#define DL_BROAE        7   // Daily broademotes
#define DL_ETRVL        8   // Daily ethereal travels
#define DL_HANDS        9   // Daily Lay on Hands
#define DL_RESURRECT        10  // Daily resurrections

#define DL_SCARES       11  // Daily scare casts
#define DL_SILENCE      12
#define DL_HARM         13  // Daily harm casts


#define DAILYLAST       13

// Object Last-time stuff
#define LT_ENCHA        0
#define LT_ENVEN        1
#define DONT_USE        2

// free             0
// free             1
// free             2
#define LT_AGGRO_ACTION     3
#define LT_TRACK        4
#define LT_MON_SCAVANGE     4
#define LT_STEAL        5
#define LT_PICKLOCK     6
#define LT_MON_WANDER       6
#define LT_SEARCH       7
#define LT_TICK         8   // Hp/Poison ticking timer
#define LT_SPELL        9
#define LT_PEEK         10
#define LT_SNEAK        11
#define LT_READ_SCROLL      12
#define LT_MAIL_ALERT   13
#define LT_HIDE         14
#define LT_TURN         15
#define LT_FRENZY       16
// free             17
// free             18
#define LT_PRAY         19
#define LT_PREPARE      20
// free             21
#define LT_PLAYER_SAVE      22
// free             23
// free             24
// free             25
#define LT_MOVED        26
// free             27
#define LT_AGE          28
// free             29
// free             30
// free             31
#define LT_SONG_PLAYED      33
#define LT_SING         35
#define LT_CHARMED      36
#define LT_MEDITATE     37
#define LT_TOUCH_OF_DEATH   38
#define LT_GORE             39
#define LT_BLOOD_SACRIFICE  40
#define LT_MOB_THIEF        41
#define LT_INVOKE       42
#define LT_SCOUT        43
#define LT_BERSERK          44

#define LT_REGENERATE       46
#define LT_DRAIN_LIFE       47
#define LT_HYPNOTIZE        48
#define LT_PLAYER_BITE      49
// free                     50
#define LT_PLAYER_SEND      51
// free             52
// free             53
// free             54
// free             55
#define LT_TRAFFIC  56
#define LT_IDENTIFY     57
#define LT_FOCUS        58
#define LT_PLAYER_STUNNED   59
// free             60
// free             61
#define LT_RP_AWARDED       62
// free             63
#define LT_UNCONSCIOUS      64
// free             65
// free             66
// free             67
// free             68
#define LT_MOB_JAILED       69
#define LT_LAY_HANDS        70
// free             71
// free             72
#define LT_NO_PKILL     73
#define LT_MOB_RENOUNCE     74
#define LT_SAVES        76

#define LT_NO_BROADCAST     77
// free             78
// free             79
// free             80
#define LT_NOMPTICK     81
#define LT_NOHPTICK     82
// free             83
#define LT_KILL_DOCTOR      84
// free             85
#define LT_LEVEL_DRAIN      86
#define LT_HOWLS        87
// free             88
#define LT_SKILL_INCREASE   89
// free             90
#define LT_ENLARGE_REDUCE   91
#define LT_ANCHOR       92
#define LT_STARSTRIKE       93
// free             94
// free             95
// free             96
// free             97
// free             98
// free             99
#define LT_TICK_SECONDARY   100 // Mp/Focus ticking
#define LT_TICK_HARMFUL     101 // Poison/Pharm etc

#define MAX_LT          103  // Incriment when you add any new LTs.


#define LT_JAILED       127 // Higher than max LT so it "ticks" when player is offline.
#define LT_MOBDEATH     126 // Ticks down offline

// Max possible LT is 126 currently. LT 127 is used for *jail - TC
#define TOTAL_LTS       127



//#define LT_MOB_PASV_GUARD LT_UNCONSCIOUS  // Mobs use LT_UNCONSCIOUS cuz they don't need it anywhere else
#define LT_RENOUNCE     LT_TOUCH_OF_DEATH
#define LT_HOLYWORD     LT_FOCUS
#define LT_SMOTHER      LT_FOCUS

// BUG: Animate & tail slap both use LT_FOCUS -> high level animate tail slaps
#define LT_ANIMATE      LT_FOCUS

#define LT_MOB_BASH     LT_IDENTIFY
#define LT_RIPOSTE      LT_MON_SCAVANGE

#define LT_DISARM       LT_FOCUS
#define LT_TAIL_SLAP        LT_FOCUS   // Mobs use LT_FOCUS on tail slap since they don't ever use focus.
#define LT_DEATH_SCREAM     LT_BERSERK   // Mobs use LT_BERSERK for death scream since they don't ever berserk.
#define LT_OUTLAW       LT_MOB_THIEF
#define LT_KICK         LT_IDENTIFY   // No one that kicks also identifies.
#define LT_MAUL         LT_IDENTIFY   // No one that mauls also identifies
#define LT_MIST         LT_MOB_THIEF
#define LT_BARKSKIN     LT_FOCUS
#define LT_PETRIFYING_GAZE  LT_MOB_JAILED

#define LT_MISTBANE     LT_FOCUS
#define LT_MOB_BREATH       LT_HYPNOTIZE
#define LT_MOB_TRAMPLE      LT_ARMOR

//#define LT_ENDURANCE        LT_FREE_ACTION
#define LT_M_AURA_ATTACK    LT_NO_PKILL

#define OLD_LT_STONESKIN    LT_MON_WANDER



// Song flags
#define SONG_HEAL       0   // Healing
#define SONG_MP         1   // Magic-Restoration
#define SONG_RESTORE        2   // Restoration
#define SONG_DESTRUCTION    3   // Destruction
#define SONG_MASS_DESTRUCTION   4   // Mass Destruction
#define SONG_BLESS      5   // Room-Bless
#define SONG_PROTECTION     6   // Room-Protection
#define SONG_FLIGHT     7   // Song of Flight
#define SONG_RECALL     8   // Song of Recall
#define SONG_SAFETY     9   // Song of Safety

#define RETURN(a,b,c)   Ply[a].io->fn = b; Ply[a].io->fnparam = c; return

#define BOOL(a)     ((a) ? 1 : 0)

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
#endif  //MAXINT
// if ltime is 0, assume uninitialized and return MAXINT.  This is so monsters
//   can have spell flags set and have them be permanent.
#define LT(a,b)     ((a)->lasttime[(b)].ltime ? (a)->lasttime[(b)].ltime + (a)->lasttime[(b)].interval : MAXINT)

#define PLYCRT(p)   ((p)->isPlayer() ? "Player" : "Monster")

//*********************************************************************
//  Below this line are customizable options for the mud
//*********************************************************************

#define MINSHOPLEVEL        16
#define SURNAME_LEVEL       13
#define ALIGNMENT_LEVEL     6


#define SUNRISE         6
#define SUNSET          20


#define ROOM_BOUND_FAILURE  2   // room to go to if getBound fails

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


// toll to charge in case none is set on exit
#define DEFAULT_TOLL        100

#define MAX_MOBS_IN_ROOM    200

// For various effects that create objects
#define SHIT_OBJ        349
#define CORPSE_OBJ      800
#define BODYPART_OBJ    21
#define STATUE_OBJ      39
#define MONEY_OBJ       0
#define TICKET_OBJ      200

// When a player forces a pet to cast, this is the delay.
#define PET_CAST_DELAY      4


#ifndef MIGNORE
extern int Crash;
/* configurable */


extern char     questions_to_email[80];

extern const int    GUILD_NONE, GUILD_INVITED, GUILD_INVITED_OFFICER, GUILD_INVITED_BANKER,
    GUILD_PEON, GUILD_OFFICER, GUILD_BANKER, GUILD_MASTER;

extern const int   GUILD_JOIN, GUILD_REMOVE, GUILD_LEVEL, GUILD_DIE;


extern int      Tablesize;
extern long     StartTime;
extern LastTime  Shutdown;
extern LastTime  Weather[5];


extern char scrollDesc [][10][20];
extern char scrollType [][2][20];


extern struct osp_t ospell[];

extern short    saving_throw_cycle[][10];
extern long     last_dust_output;


extern char *dmname[];

extern int SUPPORT_REQUIRED;
extern unsigned short Port;

extern struct osong_t osong[];

#endif

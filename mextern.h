/*
 * mextern.h
 *	 This file contains the external function and variable
 *   declarations required by the rest of the program.
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

// Pointer to config and server objects
extern Config *gConfig;
extern Server *gServer;


extern int    bHavePort;

typedef int (*SONGFN)();

#ifndef MIGNORE
extern int Crash;
/* configurable */


extern int	PORTNUM;

extern char		auth_questions_email[80];
extern char		questions_to_email[80];
extern char		register_questions_email[80];

extern const int	GUILD_NONE, GUILD_INVITED, GUILD_INVITED_OFFICER, GUILD_INVITED_BANKER,
	GUILD_PEON,	GUILD_OFFICER, GUILD_BANKER, GUILD_MASTER;

extern const int   GUILD_JOIN, GUILD_REMOVE, GUILD_LEVEL, GUILD_DIE;


extern int		Tablesize;
extern int		Cmdnum;
extern long		StartTime;
extern struct lasttime	Shutdown;
extern struct lasttime  Weather[5];
extern int		Numlockedout;

//extern plystruct Ply[PMAX];
extern class_stats_struct class_stats[CLASS_COUNT];
extern char allowedClassesStr[CLASS_COUNT + 4][16];





extern char   conjureTitles[][3][10][30];
extern char bardConjureTitles[][10][35];
extern char mageConjureTitles[][10][35];
extern creatureStats conjureStats[3][40];
extern short multiHpMpAdj[MAX_MULTICLASS][2];
extern short multiStatCycle[MAX_MULTICLASS][10];
extern short multiSaveCycle[MAX_MULTICLASS][10];


extern char scrollDesc [][10][20];
extern char scrollType [][2][20];


//Ansi/Mirc Settings
extern int Ansi[12];
extern int Mirc[9];

//extern int MAX_QUEST;




extern struct osp_t ospell[];

//extern short	level_cycle[][10];
extern short	saving_throw_cycle[][10];
//extern short	thaco_list[][30];
extern int		statBonus[40];
extern char		lev_title[][10][20];
extern char 	article[][10];
extern long		needed_exp[];
extern long		last_dust_output;


extern Dice	monk_dice[41];
extern Dice wolf_dice[41];

extern int numQuests;

extern char	*dmname[];

extern int numBans;
//extern int maxGuild;
extern int SUPPORT_REQUIRED;
extern unsigned short Port;

extern struct osong_t osong[];

#endif

#include "proto.h"
//#include <ctype.h>


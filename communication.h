/*
 * communication.h
 *	 Header file for communication
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
#ifndef COMM_H
#define COMM_H

// these are the types of communication - each has its own set of traits

// the following defines are used in communicateWith
#define COM_TELL		0
#define COM_REPLY		1
// tell the target, let the room know the target heard
#define COM_WHISPER		2
// tell the target, let the room know (and possibly understand)
#define COM_SIGN		3

// the following defines are used in communicate
#define COM_SAY			4
#define COM_RECITE		5
#define COM_YELL		6
#define COM_EMOTE		7
#define COM_GT			8


char com_text[][20] = { "sent", "replied", "whispered", "signed", "said",
	"recited", "yelled", "emoted", "group mentioned" };
char com_text_u[][20] = { "Send", "Reply", "Whisper", "Sign", "Say",
	"Recite", "Yell", "Emote", "Group mention" };


typedef struct commInfo {
	const char	*name;
	int		type;
	int		skip;
	bool	ooc;
} commInfo, *commPtr;

commInfo commList[] = 
{
	// name		type	  skip	ooc
	{ "tell",	COM_TELL,	2,	0 },
	{ "send",	COM_TELL,	2,	0 },
		
	{ "otell",	COM_TELL,	2,	true },
	{ "osend",	COM_TELL,	2,	true },

	{ "reply",	COM_REPLY,	1,	0 },
	{ "sign",	COM_SIGN,	2,	0 },
	{ "whisper",COM_WHISPER,2,	0 },

	{ NULL, 0, 0, 0 }	
};


typedef struct sayInfo {
	const char	*name;
	bool	ooc;
	bool	shout;
	bool	passphrase;
	int		type;
} sayInfo, *sayPtr;

sayInfo sayList[] = 
{
	// name		ooc		shout	passphrase	type
	{ "say",	0,		0,		0,		COM_SAY },
	{ "\"",		0,		0,		0,		COM_SAY },
	{ "'",		0,		0,		0,		COM_SAY },
		
	{ "osay",	true,	0,		0,		COM_SAY },
	{ "os",		true,	0,		0,		COM_SAY },

	{ "recite",	0,		0,		true,	COM_RECITE },
		
	{ "yell",	0,		true,	0,		COM_YELL },

	{ "emote",	0,		0,		0,		COM_EMOTE },
	{ ":",		0,		0,		0,		COM_EMOTE },

	{ "gtalk",	0,		0,		0,		COM_GT },
	{ "gt",		0,		0,		0,		COM_GT },
	{ "gtoc",	true,	0,		0,		COM_GT },

	{ NULL, 0, 0, 0, 0 }	
};



// using these defines require more complicated authorization.
#define COM_CLASS		1
#define COM_RACE		2
#define COM_CLAN		3



typedef struct channelInfo {
	const char	*channelName;		// Name of the channel
	bstring color;
	const char	*displayFmt;		// Display string for the channel
	int		minLevel;				// Minimum level to use this channel
	int		maxLevel;				// Maximum level to use this channel
	bool	eaves;					// does this channel show up on eaves?
	bool	(*canSee)(const Creature *);	// Can this person use this channel?
	bool	(*canUse)(Player *);	// Can this person use this channel?
	// these are used to determine canHear:
	// every field must be satisfied (or empty) for them to hear the channel
	bool	(*canHear)(Socket*);	// a function that MUST return
	int		flag;					// a flag that MUST be set
	int		not_flag;				// a flag that MUST NOT be set
	int		type;					// for more complicated checks
} channelInfo, *channelPtr;

channelInfo channelList[] = 
{
//     Name       	Color			Format 										MIN	MAX eaves	canSee		canUse				canHear		flag	not flag				type
	{ "broadcast",	"*CC:BROADCAST*",	"### %M broadcasted, \"%s\"",			2, 	-1,	false,	0,			canCommunicate,		0,			0,		P_NO_BROADCASTS,		0 },
	{ "broad",		"*CC:BROADCAST*",	"### %M broadcasted, \"%s\"",			2,  -1,	false,	0,			canCommunicate,		0,			0,		P_NO_BROADCASTS,		0 },
	{ "bro",		"*CC:BROADCAST*",	"### %M broadcasted, \"%s\"",			2,  -1,	false,	0,			canCommunicate,		0,			0,		P_NO_BROADCASTS,		0 },
	{ "bemote",		"*CC:BROADCAST*",	"*** %M %s.",							2, 	-1,	false,	0,			canCommunicate,		0,			0,		P_NO_BROADCASTS,		0 },
	{ "broademote",	"*CC:BROADCAST*",	"*** %M %s.",							2, 	-1,	false,	0,			canCommunicate,		0,			0,		P_NO_BROADCASTS,		0 },

	{ "gossip",		"*CC:GOSSIP*",		"(Gossip) %M sent, \"%s\"",				2,	-1,	false,	0,			canCommunicate,		0,			0,		P_IGNORE_GOSSIP,		0 },
	{ "ptest",		"*CC:PTEST*",		"[P-Test] %M sent, \"%s\"",				-1,	-1,	false,	isPtester,	0,					isPtester,	0,		0,						0 },
	{ "newbie",		"*CC:NEWBIE*",		"[Newbie]: *** %R just sent, \"%s\"",	1,	 4,	false,	0,			canCommunicate,		0,			0,		P_IGNORE_NEWBIE_SEND,	0 },

	{ "dm",			"*CC:DM*",			"(DM) %R sent, \"%s\"",					-1,	-1,	false,	isDm,		0,					isDm,		0,		0,						0 },
	{ "admin",		"*CC:ADMIN*",		"(Admin) %R sent, \"%s\"",				-1,	-1,	false,	isAdm,		0,					isAdm,		0,		0,						0 },
	{ "*s",			"*CC:SEND*",		"=> %R sent, \"%s\"",					-1,	-1,	false,	isCt,		0,					isCt,		0,		0,						0 },
	{ "*send",		"*CC:SEND*",		"=> %R sent, \"%s\"",					-1,	-1,	false,	isCt,		0,					isCt,		0,		0,						0 },
	{ "*msg",		"*CC:MESSAGE*",		"-> %R sent, \"%s\"",					-1,	-1,	false,	isStaff,	0,					isStaff,	0,		P_NO_MSG,				0 },
	{ "*wts",		"*CC:WATCHER*",		"-> %R sent, \"%s\"",					-1,	-1,	false,	isWatcher,	0,					isWatcher,	0,		P_NO_WTS,				0 },

	{ "cls",		"*CC:CLASS*",		"### %R sent, \"%s\".",					-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_CLASS_SEND,	COM_CLASS },
	{ "classsend",	"*CC:CLASS*",		"### %R sent, \"%s\".",					-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_CLASS_SEND,	COM_CLASS },
	{ "clem",		"*CC:CLASS*",		"### %R %s.",							-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_CLASS_SEND,	COM_CLASS },
	{ "classemote",	"*CC:CLASS*",		"### %R %s.",							-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_CLASS_SEND,	COM_CLASS },

	{ "racesend",	"*CC:RACE*",		"### %R sent, \"%s\".",					-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_RACE_SEND,		COM_RACE },
	{ "raemote",	"*CC:RACE*",		"### %R %s.",							-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_RACE_SEND,		COM_RACE },

	{ "clansend",	"*CC:CLAN*",		"### %R sent, \"%s\".",					-1, -1, true,	0,			canCommunicate, 	0,			0,		P_IGNORE_CLAN,			COM_CLAN },

	{ NULL,			"",					NULL,									0,	0,	false,	0,			0,					0,			0,		0,						0 }
};


#endif

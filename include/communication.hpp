/*
 * communication.h
 *   Header file for communication
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
#ifndef COMM_H
#define COMM_H

#include "commands.hpp"
#include "flags.hpp"
#include "proto.hpp"

class Creature;
class Player;
class Socket;

// these are the types of communication - each has its own set of traits

// the following defines are used in communicateWith
#define COM_TELL        0
#define COM_REPLY       1
// tell the target, let the room know the target heard
#define COM_WHISPER     2
// tell the target, let the room know (and possibly understand)
#define COM_SIGN        3

// the following defines are used in communicate
#define COM_SAY         4
#define COM_RECITE      5
#define COM_YELL        6
#define COM_EMOTE       7
#define COM_GT          8

// using these defines require more complicated authorization.
#define COM_CLASS       9
#define COM_RACE        10
#define COM_CLAN        11

extern const long IN_GAME_WEBHOOK;

extern char com_text[][20];
extern char com_text_u[][20];

typedef struct commInfo {
    const char  *name;
    int     type;
    int     skip;
    bool    ooc;
} commInfo, *commPtr;

extern commInfo commList[];

typedef struct sayInfo {
    const char  *name;
    bool    ooc;
    bool    shout;
    bool    passphrase;
    int     type;
} sayInfo, *sayPtr;

extern sayInfo sayList[];


typedef struct channelInfo {
    const char  *channelName;       // Name of the channel
    bool    useLanguage;            // Should this channel use languages?
    std::string color;
    const char  *displayFmt;        // Display string for the channel
    int     minLevel;               // Minimum level to use this channel
    int     maxLevel;               // Maximum level to use this channel
    bool    eaves;                  // does this channel show up on eaves?
    bool    (*canSee)(const std::shared_ptr<Creature>& );    // Can this person use this channel?
    bool    (*canUse)(const std::shared_ptr<Player>& );    // Can this person use this channel?
    // these are used to determine canHear:
    // every field must be satisfied (or empty) for them to hear the channel
    bool    (*canHear)(Socket*);    // a function that MUST return
    int     flag;                   // a flag that MUST be set
    int     not_flag;               // a flag that MUST NOT be set
    int     type;                   // for more complicated checks
    long    discordWebhookID;       // DiscordWebhook this should be sent to, if any.  -1 for None
    long    discordChannelID;       // DiscordChannelID, if any.  -1 for None.
} channelInfo, *channelPtr;

extern channelInfo channelList[];


void sendGlobalComm(const std::shared_ptr<Player> player, const std::string &text, const std::string &extra, unsigned int check,
                    const channelInfo *chan, const std::string &etxt, const std::string &oocName, const std::string &icName);


channelPtr getChannelByName(const std::shared_ptr<Player>& player, const std::string &chanStr);
channelPtr getChannelByDiscordChannel(unsigned long discordChannelID);


#endif

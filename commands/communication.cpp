/*
 * communication.cpp
 *   Functions used for communication on the mud
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

#include <fmt/format.h>                          // for format
#include <strings.h>                             // for strncasecmp
#include <boost/algorithm/string/predicate.hpp>  // for icontains
#include <boost/algorithm/string/replace.hpp>    // for replace_all
#include <boost/iterator/iterator_traits.hpp>    // for iterator_value<>::type
#include <cstdio>                                // for sprintf, size_t
#include <cstring>                               // for strcpy, strlen, strcmp
#include <deque>                                 // for _Deque_iterator
#include <list>                                  // for list, operator==
#include <map>                                   // for operator==, _Rb_tree...
#include <set>                                   // for set, set<>::iterator
#include <sstream>                               // for operator<<, basic_os...
#include <string>                                // for string, allocator
#include <string_view>                           // for operator<<, string_view
#include <utility>                               // for pair

#include "clans.hpp"                             // for Clan
#include "cmd.hpp"                               // for cmd
#include "color.hpp"                             // for escapeColor
#include "commands.hpp"                          // for isPtester, cmdNoExist
#include "communication.hpp"                     // for channelInfo, commInfo
#include "config.hpp"                            // for Config, gConfig, Cla...
#include "creatureStreams.hpp"                   // for Streamable, ColorOff
#include "deityData.hpp"                         // for DeityData
#include "flags.hpp"                             // for P_AFK, P_NO_BROADCASTS
#include "global.hpp"                            // for CreatureClass, LCOMMON
#include "group.hpp"                             // for CreatureList, GROUP_...
#include "move.hpp"                              // for getRoom
#include "mudObjects/container.hpp"              // for PlayerSet
#include "mudObjects/creatures.hpp"              // for Creature
#include "mudObjects/exits.hpp"                  // for Exit
#include "mudObjects/monsters.hpp"               // for Monster
#include "mudObjects/players.hpp"                // for Player
#include "mudObjects/rooms.hpp"                  // for BaseRoom, ExitList
#include "playerClass.hpp"                       // for PlayerClass
#include "proto.hpp"                             // for broadcast, get_langu...
#include "raceData.hpp"                          // for RaceData
#include "random.hpp"                            // for Random
#include "server.hpp"                            // for Server, gServer, Pla...
#include "socket.hpp"                            // for Socket, MXP_BEG, MXP...
#include "structs.hpp"                           // for Command
#include "xml.hpp"                               // for loadPlayer

class Guild;
bool canCommunicate(const std::shared_ptr<Player>& player);

char com_text[][20] = {"sent", "replied", "whispered", "signed", "said",
                       "recited", "yelled", "emoted", "group mentioned" };
char com_text_u[][20] = { "Send", "Reply", "Whisper", "Sign", "Say",
                          "Recite", "Yell", "Emote", "Group mention" };


commInfo commList[] = {
    // name     type      skip  ooc
    { "tell",   COM_TELL,   2,  false },
    { "send",   COM_TELL,   2,  false },

    { "otell",  COM_TELL,   2,  true },
    { "osend",  COM_TELL,   2,  true },

    { "reply",  COM_REPLY,  1,  false },
    { "sign",   COM_SIGN,   2,  false },
    { "whisper",COM_WHISPER,2,  false },

    { nullptr, 0, 0, false }
};


sayInfo sayList[] = {
    // name     ooc     shout   passphrase  type
    { "say",    false,      false,      false,      COM_SAY },
    { "\"",     false,      false,      false,      COM_SAY },
    { "'",      false,      false,      false,      COM_SAY },

    { "osay",   true,   false,      false,      COM_SAY },
    { "os",     true,   false,      false,      COM_SAY },

    { "recite", false,      false,      true,   COM_RECITE },

    { "yell",   false,      true,   false,      COM_YELL },

    { "emote",  false,      false,      false,      COM_EMOTE },
    { ":",      false,      false,      false,      COM_EMOTE },

    { "gtalk",  false,      false,      false,      COM_GT },
    { "gt",     false,      false,      false,      COM_GT },
    { "gtoc",   true,   false,      false,      COM_GT },

    { nullptr, false, false, false, 0 }
};

const long IN_GAME_WEBHOOK = 886681065878605855;

channelInfo channelList[] = {
    //     Name         OOC     Color               Format                                              MIN MAX eaves   canSee                  canUse      canHear     flag    not flag                type
    { "broadcast",  true,  "*CC:BROADCAST*",    "### *IC-NAME* broadcasted, \"*TEXT*\"",            2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_NO_BROADCASTS,        0, IN_GAME_WEBHOOK, 886681021041500170},
    { "broad",      true,  "*CC:BROADCAST*",    "### *IC-NAME* broadcasted, \"*TEXT*\"",            2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_NO_BROADCASTS,        0, IN_GAME_WEBHOOK, -1},
    { "bro",        true,  "*CC:BROADCAST*",    "### *IC-NAME* broadcasted, \"*TEXT*\"",            2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_NO_BROADCASTS,        0, IN_GAME_WEBHOOK, -1},
    { "bemote",     true,  "*CC:BROADCAST*",    "*** *IC-NAME* *TEXT*.",                            2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_NO_BROADCASTS,        COM_EMOTE, IN_GAME_WEBHOOK, -1},
    { "broademote", true,  "*CC:BROADCAST*",    "*** *IC-NAME* *TEXT*.",                            2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_NO_BROADCASTS,        COM_EMOTE, IN_GAME_WEBHOOK, -1},

    { "gossip",     true,  "*CC:GOSSIP*",       "(Gossip) *IC-NAME* sent, \"*TEXT*\"",              2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_GOSSIP,        0, 886678176099627100, 886678132327862332},
    { "sports",     true,  "*CC:SPORTS*",       "(Sports) *IC-NAME* sent, \"*TEXT*\"",              2,  -1, false,  nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_SPORTS,        0, 1041878273816281150, 976930886559883264},
    { "ptest",      false,   "*CC:PTEST*",      "[P-Test] *IC-NAME* sent, \"*TEXT*\"",              -1, -1, false,  isPtester,         nullptr,            isPtester,         0,      0,              0, -1, -1},
    { "newbie",     false,   "*CC:NEWBIE*",     "[Newbie]: *** *OOC-NAME* just sent, \"*TEXT*\"",   1,   4, false,  nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_NEWBIE_SEND,   0, 886681403146788914, 886681367885279282},

    { "dm",         false,   "*CC:DM*",         "(DM) *OOC-NAME* sent, \"*TEXT*\"",                 -1, -1, false,  isDm,              nullptr,            isDm,              0,      0,              0, 886471850715136010, 886050341035061308},
    { "admin",      false,   "*CC:ADMIN*",      "(Admin) *OOC-NAME* sent, \"*TEXT*\"",              -1, -1, false,  isAdm,             nullptr,            isAdm,             0,      0,              0, -1, -1},
    { "*s",         false,   "*CC:SEND*",       "=> *OOC-NAME* sent, \"*TEXT*\"",                   -1, -1, false,  isCt,              nullptr,            isCt,              0,      0,              0, 886677295312568440, 409888057916129281},
    { "*send",      false,   "*CC:SEND*",       "=> *OOC-NAME* sent, \"*TEXT*\"",                   -1, -1, false,  isCt,              nullptr,            isCt,              0,      0,              0, 886677295312568440, -1},
    { "*msg",       false,   "*CC:MESSAGE*",    "-> *OOC-NAME* sent, \"*TEXT*\"",                   -1, -1, false,  isStaff,           nullptr,            isStaff,           0,      P_NO_MSG,               0, 948453398078955552, 942225119340822528},
    { "*wts",       false,   "*CC:WATCHER*",    "-> *OOC-NAME* sent, \"*TEXT*\"",                   -1, -1, false,  isWatcher,         nullptr,            isWatcher,         0,      P_NO_WTS,               0, -1, -1},

    { "cls",        true,   "*CC:CLASS*",       "### *OOC-NAME* sent, \"*TEXT*\".",                 -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_CLASS_SEND,    COM_CLASS, -1, -1},
    { "classsend",  true,   "*CC:CLASS*",       "### *OOC-NAME* sent, \"*TEXT*\".",                 -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_CLASS_SEND,    COM_CLASS, -1, -1},
    { "clem",       false,  "*CC:CLASS*",       "### *OOC-NAME* *TEXT*.",                           -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_CLASS_SEND,    COM_CLASS, -1, -1},
    { "classemote", false,  "*CC:CLASS*",       "### *OOC-NAME* *TEXT*.",                           -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_CLASS_SEND,    COM_CLASS, -1, -1},

    { "racesend",   true,   "*CC:RACE*",        "### *OOC-NAME* sent, \"*TEXT*\".",                 -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_RACE_SEND,     COM_RACE, -1, -1},
    { "raemote",    false,  "*CC:RACE*",        "### *OOC-NAME* *TEXT*.",                           -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_RACE_SEND,     COM_RACE, -1, -1},

    { "clansend",   true,   "*CC:CLAN*",        "### *OOC-NAME* sent, \"*TEXT*\".",                 -1, -1, true,   nullptr,          canCommunicate,     nullptr,    0,      P_IGNORE_CLAN,          COM_CLAN, -1, -1},

    { nullptr,         false,  "",              nullptr,                                            0,  0,  false,  nullptr,          nullptr,     nullptr,    0,      0,             0, -1 , -1},
};


//*********************************************************************
//                      confusionChar
//*********************************************************************

char confusionChar() {
    char n;
    switch(Random::get(0,10)) {
    case 0:
        // random upper case
        return(Random::get<char>(65, 90));
    case 1:
    case 2:
        // random character
        n = Random::get<char>(0,57);
        // weight all the characters in the different ranges evenly
        if(n <= 31) {
            // Random::get(91, 121);      31 chars
            return(n + (char)91);
        } else if(n <= 38) {
            // Random::get(58, 64);       7 chars
            return(n - (char)31 + (char)58);
        } else if(n <= 54) {
            // Random::get(32, 47);       16 chars
            return(n - (char)31 - (char)7 + (char)32);
        } else {
            // Random::get(123, 125);     3 chars
            return(n - (char)31 - (char)7 -(char) 16 + (char)123);
        }
    case 3:
        // random number
        return(Random::get<char>(48, 57));
        break;
    default:
        break;
    }
    // random lower case
    return(Random::get<char>(97, 122));
}

//*********************************************************************
//                      confusionText
//*********************************************************************
// make the player say gibberish instead of actual words

std::string confusionText(const std::shared_ptr<Creature>& speaker, std::string text) {
    if( (!speaker->isEffected("confusion") && !speaker->isEffected("drunkenness")) ||
        speaker->isStaff()
    )
        return(text);

    bool scramble = Random::get(0,1);

    for(char &i : text) {
        if(i == ' ') {
            scramble = !Random::get(0,3);
            // sometimes scramble spaces, too
            if(!Random::get(0,8))
                i = confusionChar();
        } else {

            if(scramble || !Random::get(0,8))
                i = confusionChar();

        }
    }
    return(text);
}


//*********************************************************************
//                      getFullstrText
//*********************************************************************
// general function that gets a cmnd->fullstr and returns the portion
// of the text that will get sent to other player(s)
// skip = 1 when we only have 1 command to skip
//    Ex: say hello all
// skip = 2 when we have 2 commands to skip
//    Ex: tell bob hello there

std::string getFullstrTextTrun(std::string str, int skip, char toSkip, bool colorEscape) {
    return(getFullstrText(std::move(str), skip, toSkip, colorEscape, true));
}

std::string getFullstrText(std::string str, int skip, char toSkip, bool colorEscape, bool truncate) {
    int i=0, n = str.length() - 1;

    // first of all, trim both directions
    while(i <= n && str.at(i) == toSkip)
        i++;
    while(n >= 0 && str.at(n) == toSkip)
        n--;

    while(skip > 0) {
        // skip a set of spaces
        for(; i < (int)str.length()-1; i++)
            if(str.at(i) == toSkip && str.at(i+1) != toSkip)
                break;
        i++;
        skip--;
    }


    // these numbers can't overlap
    if(i > n)
        str = "";
    else {
        str = str.substr(0, n+1);
        str = str.substr(i);

        if(truncate) {
            // skip a set of spaces
            for(i=0; i < (int)str.length()-1; i++) {
                if(str.at(i) == toSkip && str.at(i+1) != toSkip) {
                    i--;
                    break;
                }
            }
            i++;

            // these numbers can't overlap
            str = str.substr(0, i);
        }
    }

    if(colorEscape)
        str = escapeColor(str);
    return(str);
}

//*********************************************************************
//                      communicateWith
//*********************************************************************
// This is the base function for communication with a target player.
// The commands tell, whisper, reply, and sign are all routed through here.

int communicateWith(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::string text;
    std::shared_ptr<Player> target=nullptr;
    int     i=0, found=0;
    char    ooc_str[10];

    // reuse text
    text = cmnd->myCommand->getName();

    // get us a channel to use!
    commPtr chan = nullptr;
    while(commList[i].name != nullptr) {
        if(!strcmp(text.c_str(), commList[i].name)) {
            chan = &commList[i];
            break;
        }
        i++;
    }
    if(!chan) {
        player->print("Unknown channel.\n");
        return(0);
    }

    strcpy(ooc_str, (chan->ooc ? "[ooc]: " : ""));


    // extra checks for reply and sign
    if(chan->type == COM_REPLY) {
        strcpy(cmnd->str[1], player->getLastCommunicate().c_str());
        if(!strlen(cmnd->str[1])) {
            player->print("You have nobody to reply to right now.\n");
            return(0);
        }
    } else if(chan->type == COM_SIGN) {
        if(player->getRace() != DARKELF && !player->isStaff()) {
            player->print("Only dark elves may communicate by signing.\n");
            return(0);
        }
    }


    // the basic communication checks all commands must obey
    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < chan->skip) {
        player->print("%s to whom?\n", com_text_u[chan->type]);
        return(0);
    }

    if(!player->flagIsSet(P_SECURITY_CHECK_OK)) {
        player->print("You may not do that yet.\n");
        return(0);
    }

    // silence!
    if(chan->type != COM_SIGN) {
        if(player->flagIsSet(P_DM_SILENCED)) {
            player->printColor("^CYou are unable to move your fingers.\n");
            return(0);
        }
        if(!player->canSpeak()) {
            player->printColor("^yYou cannot speak.\n");
            return(0);
        }
    } else {
        if(player->flagIsSet(P_DM_SILENCED)) {
            player->printColor("^yYou are unable to move your lips.\n");
            return(0);
        }
    }


    // run the player table
    cmnd->str[1][0] = up(cmnd->str[1][0]);
    for(const auto& [pId, ply] : gServer->players) {
        if(!ply->isConnected())
            continue;
        // these two need vicinity
        if(chan->type == COM_SIGN || chan->type == COM_WHISPER)
            if(!player->inSameRoom(ply))
                continue;
        if(!player->canSee(ply))
            continue;
        if(ply->getName() == cmnd->str[1]) {
            target = ply;
            break;
        }
        if(!strncmp(ply->getCName(), cmnd->str[1], strlen(cmnd->str[1]))) {
            target = ply;
            found++;
        }
    }

    if(found > 1) {
        player->print("More than one person with that name.\n");
        return(0);
    }

    if(!target || !player->canSee(target)) {
        player->print("%s to whom?\n", com_text_u[chan->type]);
        return(0);
    }



    // higher level communication checks
    if(player == target && !player->flagIsSet(P_CAN_FLASH_SELF) && !player->isStaff()) {
        player->print("Talking to yourself is a sign of insanity.\n");
        return(0);
    }

    if(player->getClass() == CreatureClass::BUILDER && !target->isStaff()) {
        player->print("You may only communicate with staff!\n");
        return(0);
    }

    // polite ignore message for watchers
    if(!player->isCt() && target->flagIsSet(P_NO_TELLS)) {
        if(target->isPublicWatcher())
            player->print("%s is busy at the moment. Please mudmail %s.\n", target->getCName(), target->himHer());
        else
            player->print("%s is ignoring everyone.\n", target->getCName());
        return(0);
    }


    // staff can always message AFK people
    if( target->flagIsSet(P_AFK) &&
        !player->checkStaff("%s is currently afk.\n", target->getCName()))
        return(0);

    if(chan->type != COM_SIGN && target->isEffected("deafness") && !player->isStaff()) {
        player->print("%s is deaf and cannot hear anything.\n", target->getCName());
        return(0);
    }


    if( target->isIgnoring(player->getName()) &&
        !target->checkStaff("%s is ignoring you.\n", target->getCName())
    )
        return(0);

    if(chan->type == COM_SIGN || chan->type == COM_WHISPER)
        player->unhide();

    if(chan->type == COM_SIGN) {
        if(!target->isStaff()) {
            if(target->getRace() != DARKELF && target->getClass() < CreatureClass::BUILDER) {
                player->print("%s doesn't understand dark elven sign language.\n", target->upHeShe());
                return(0);
            }
            if(target->isEffected("blindness")) {
                player->print("%M is blind! %s can't see what you're signing.\n", target.get(), target->upHeShe());
                return(0);
            }
            if(player->isInvisible() && !target->isEffected("detect-invisible")) {
                player->print("You are invisible! %s cannot see what you are signing.\n", target->getCName());
                return(0);
            }
        }
    } else {
        if( !target->languageIsKnown(LUNKNOWN+player->current_language) &&
            !player->isStaff() &&
            !target->isEffected("comprehend-languages")
        ) {
            player->print("%s doesn't speak your current language.\n", target->upHeShe());
            return(0);
        }
    }

    // spam check
    if(player->checkForSpam())
        return(0);


    // reuse text
    text = getFullstrText(cmnd->fullstr, chan->skip, ' ', true);
    if(text.empty()) {
        player->print("%s what?\n", com_text_u[chan->type]);
        return(0);
    }
    text = confusionText(player, text);

    player->printColor("^cYou %s \"%s%s\" to %N.\n", com_text[chan->type], ooc_str, text.c_str(), target.get());


    if(chan->type == COM_WHISPER)
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M whispers something to %N.", player.get(), target.get());


    if(!target->isGagging(player->getName())) {
        target->printColor("%s%s%M %s%s, \"%s%s\".\n",
            target->customColorize("*CC:TELL*").c_str(),
            chan->type == COM_WHISPER || chan->type == COM_SIGN ? "" : "### ",
            player.get(),
            chan->type == COM_WHISPER || chan->type == COM_SIGN ? com_text[chan->type] : "just flashed",
            chan->type == COM_WHISPER || chan->type == COM_SIGN ? " to you" : "",
            ooc_str, text.c_str());
    } else {
        if(!player->isDm() && !target->isDm())
            broadcast(watchingEaves, "^E--- %s TRIED sending to %s, \"%s\".",
                player->getCName(), target->getCName(), text.c_str());
    }


    // sign doesnt use reply anyway
    if(chan->type != COM_SIGN) {

        if(!target->isStaff() &&
            ( ( player->isEffected("incognito") && !player->inSameRoom(target) ) ||
                ( player->isInvisible() && !target->isEffected("detect-invisible") ) ||
                ( player->isEffected("mist") && !target->isEffected("true-sight") ) ||
                player->flagIsSet(P_DM_INVIS) ||
                target->flagIsSet(P_UNCONSCIOUS) ||
                target->isBraindead() ||
                target->flagIsSet(P_LINKDEAD)
            )
        ) {
            player->print("%s will be unable to reply.", target->upHeShe());
            if(target->flagIsSet(P_LINKDEAD))
                player->print(" %s is linkdead.", target->upHeShe());
            player->print("\n");

            if(player->flagIsSet(P_ALIASING))
                player->print("Sent from: %s.\n", player->getAlias()->getCName());
        } else if(player->flagIsSet(P_IGNORE_ALL)) {
            player->print("You are ignoring all, as such they will be unable to respond.\n");
        }

    } else {

        for(const auto& pIt: player->getRoomParent()->players) {
            if(auto ply = pIt.lock()) {
                if (ply != player && ply != target && !ply->isEffected("blindness")) {
                    if (ply->getRace() == DARKELF || ply->isStaff())
                        ply->print("%M signed, \"%s\" to %N.\n", player.get(), text.c_str(), target.get());
                    else
                        ply->print("%M signed something in dark elven to %N.\n", player.get(), target.get());
                }
            }
        }

    }

    if(player->isDm() || target->isDm())
        return(0);

    broadcast(watchingEaves, "^E--- %s %s to %s, \"%s%s\".", player->getCName(),
        com_text[chan->type], target->getCName(), ooc_str, text.c_str());

    if(chan->type != COM_SIGN)
        target->setLastCommunicate(player->getName());
    return(0);
}


//*********************************************************************
//                      commTarget
//*********************************************************************
// function that does the actual printing of message for below

void commTarget(const std::shared_ptr<Creature>& player, const std::shared_ptr<Player>& target, int type, bool ooc, int lang, std::string_view text, std::string speak, char *ooc_str, bool anon) {
    std::ostringstream out;

    if(!target || target->flagIsSet(P_UNCONSCIOUS))
        return;
    if(target->isEffected("deafness") && !player->isStaff())
        return;

    if(type != COM_GT)
        speak += "s";

    if(type == COM_EMOTE) {
        out << player->getCrtStr(target, CAP) << " " << text << "\n";
    } else if( ooc ||
        target->languageIsKnown(LUNKNOWN+lang) ||
        target->isStaff() ||
        target->isEffected("comprehend-languages"))
    {

        if(type == COM_GT)
            out << target->customColorize("*CC:GROUP*").c_str() << "### ";

        if(ooc || lang == LCOMMON) {
            if(anon) out << "Someone";
            else     out << player->getCrtStr(target, CAP);
            out << " " << speak;
        } else {
            if(anon) out << "Someone";
            else     out << player->getCrtStr(target, CAP);

            out << " " << speak << " in " << get_language_adj(lang);
        }
        out << ", \"";

        out << ooc_str << text;
        out << "\".\n";

    } else {
        if(anon) out << "Someone";
        else     out << player->getCrtStr(target, CAP);

        out << " " << speak << " something in " << get_language_adj(lang) << ".\n";
    }
    *target << ColorOn << out.str() << ColorOff;

}

//*********************************************************************
//                      communicate
//*********************************************************************
// This function allows the player to say something to all the other
// people in the room (or nearby rooms).

int pCommunicate(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(communicate(player, cmnd));
}

int communicate(const std::shared_ptr<Creature>& creature, cmd* cmnd) {
    std::shared_ptr<const Player> player=nullptr;
    std::string text, name;
    std::shared_ptr<Creature>owner=nullptr;
    std::shared_ptr<Player> pTarget=nullptr;
    std::shared_ptr<BaseRoom> room=nullptr;

    int     i=0, lang;
    char    speak[35], ooc_str[10];
    if(creature->isPet())
        owner = creature->getMaster();
    else
        owner = creature;
    player = owner->getAsConstPlayer();

    // reuse text
    text = cmnd->myCommand->getName();

    // get us a channel to use!
    sayPtr chan = nullptr;
    while(sayList[i].name != nullptr) {
        if(!strcmp(text.c_str(), sayList[i].name)) {
            chan = &sayList[i];
            break;
        }
        i++;
    }
    if(!chan) {
        creature->print("Unknown channel.\n");
        return(0);
    }

    strcpy(ooc_str, (chan->ooc ? "[ooc]: " : ""));
    owner->clearFlag(P_AFK);

    if(!creature->ableToDoCommand())
        return(0);

    if(!creature->canSpeak() || (owner && owner != creature && !owner->canSpeak())) {
        creature->printColor("^yYour lips move but no sound comes forth.\n");
        return(0);
    }
    // reuse text
    text = getFullstrText(cmnd->fullstr, 1, ' ', true);
    if(text.empty()) {
        creature->print("%s what?\n", com_text_u[chan->type]);
        return(0);
    }
    text = confusionText(owner, text);

    lang = owner->current_language;

    if(chan->ooc)
        strcpy(speak, "say");
    else if(chan->type == COM_RECITE)
        strcpy(speak, "recite");
    else if(chan->type == COM_GT)
        strcpy(speak, "group mentioned");
    else if(chan->type == COM_YELL) {
        strcpy(speak, "yell");
        text += "!";
    } else
        strcpy(speak, get_language_verb(lang));


    if(chan->type == COM_GT) {
        // Group Tell is being handled slightly differently as it now uses a std::list instead of ctags
        // Adapt the other communication methods to use this once they've been moved to a std::list as well

        Group* group = creature->getGroup();
        if(group) {
            auto it = group->members.begin();
            while(it != group->members.end()) {
                if(auto crt = (*it).lock()) {
                    it++;
                    pTarget = crt->getAsPlayer();
                    if(!pTarget) continue;
                    // GT prints to the player!
                    if(pTarget == creature && chan->type != COM_GT)
                        continue;

                    if(pTarget->isGagging(creature->isPet() ? creature->getMaster()->getCName() : creature->getCName()))
                        continue;

                    if(pTarget->getGroupStatus() < GROUP_MEMBER) continue;

                    commTarget(creature, pTarget, chan->type, chan->ooc, lang, text, speak, ooc_str, false);
                } else {
                    it = group->members.erase(it);
                }
            }

        } else {
            creature->print("You are not in a group.\n");
            return(0);
        }

    } else {
        creature->unhide();

        if(chan->type == COM_EMOTE) {

            creature->printColor("You emote: %s.\n", text.c_str());

        } else {
            char intro[2046];
            if(chan->ooc || lang == LCOMMON)
                sprintf(intro, "You %s,", speak);
            else
                sprintf(intro, "You %s in %s,", speak, get_language_adj(lang));

            creature->printColor("%s%s \"%s%s\".\n^x", ((!chan->ooc && creature->flagIsSet(P_LANGUAGE_COLORS)) ? get_lang_color(lang) : ""),
                    intro, ooc_str, text.c_str());

        }

        for(const auto& pIt: creature->getRoomParent()->players) {
            if(auto ply = pIt.lock()) {
                if (chan->shout)
                    ply->wake("Loud noises disturb your sleep.", true);

                // GT prints to the player!
                if (ply == creature && chan->type != COM_GT)
                    continue;

                if (ply->isGagging(creature->isPet() ? creature->getMaster()->getName() : creature->getName()))
                    continue;

                commTarget(creature, ply, chan->type, chan->ooc, lang, text, speak, ooc_str, false);
            }
        }

        if(chan->shout) {
            // Because of multiple exits leading to the same room, we will keep
            // track of who has heard us shout
            std::list<std::shared_ptr<Socket>> listeners;
            std::list<std::shared_ptr<Socket>>::iterator it;
            bool    heard = false;

            // This allows a player to yell something that will be heard
            // not only in their room, but also in all rooms adjacent to them. In
            // the adjacent rooms, however, people will not know who yelled.


            for(const auto& exit : creature->getRoomParent()->exits) {
                room = nullptr;
                i=0;
                PlayerSet::iterator pIt, pEnd;
                // don't shout through closed doors
                if(!exit->flagIsSet(X_CLOSED))
                    Move::getRoom(nullptr, exit, room, true);

                // the same-room checks aren't run in getRoom because
                // we don't send a creature.
                if(room) {
                    if(creature->getParent() && room == creature->getParent())
                        continue;
                    pIt = room->players.begin();
                    pEnd = room->players.end();
                } else
                    continue;

                while(pIt != pEnd) {
                    pTarget = (*pIt++).lock();

                    if(!pTarget)
                        continue;

                    pTarget->wake("Loud noises disturb your sleep.", true);
                    if(pTarget->isGagging(creature->getName()))
                        continue;

                    // have they already heard us yell?
                    heard = false;

                    for(it = listeners.begin() ; it != listeners.end() ; it++) {
                        if((*it) == pTarget->getSock()) {
                            heard = true;
                            break;
                        }
                    }

                    if(!heard) {
                        listeners.push_back(pTarget->getSock());
                        commTarget(creature, pTarget, chan->type, chan->ooc, lang, text, speak, ooc_str, true);
                    }
                }
            }
            listeners.clear();
        }

        if(chan->passphrase) {

            for(const auto& exit : creature->getRoomParent()->exits) {
                // got the phrase right?
                if(!exit->getPassPhrase().empty() && exit->getPassPhrase() == text) {
                    // right language?
                    if(!exit->getPassLanguage() || lang == exit->getPassLanguage()) {
                        // even needs to be open?
                        if(exit->flagIsSet(X_LOCKED)) {
                            broadcast((std::shared_ptr<Socket> )nullptr, creature->getRoomParent(), "The %s opens!", exit->getCName());
                            exit->clearFlag(X_LOCKED);
                            exit->clearFlag(X_CLOSED);

                            if(!exit->getOpen().empty()) {
                                if(exit->flagIsSet(X_ONOPEN_PLAYER)) {
                                    creature->print("%s.\n", exit->getOpen().c_str());
                                } else {
                                    broadcast((std::shared_ptr<Socket> )nullptr, creature->getRoomParent(), exit->getOpen().c_str());
                                }
                            }

                        }
                    }
                }
            }
        }

    }
    // DMs still broadcast to eaves on GT
    if(creature->isDm() && chan->type != COM_GT)
        return(0);


    if(creature->isPet()) {
        name = creature->getMaster()->getName() + "'s " + creature->getName();
    } else
        name = creature->getName();

    if(chan->type == COM_EMOTE)
        broadcast(watchingSuperEaves, "^E--- %s %s.", name.c_str(), text.c_str());
    else
        broadcast(watchingSuperEaves, "^E--- %s %s, \"%s%s\" in %s.", name.c_str(), com_text[chan->type],
            ooc_str, text.c_str(), get_language_adj(lang));

    std::shared_ptr<Player> pPlayer = creature->getAsPlayer();
    // spam check
    if(pPlayer)
        pPlayer->checkForSpam();

    return(0);
}

std::string mxpTag(std::string_view str) {
    return( fmt::format("{}{}{}", MXP_BEG, str, MXP_END));
}
//*********************************************************************
//                      channel
//**********************************************************************
// This function is used as a base for all global communication channels

int channel(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::string text, chanStr, extra;
    size_t i = 0;
    int check=0, skip=1;
    const Guild* guild=nullptr;

    const int max_class = static_cast<int>(CreatureClass::CLASS_COUNT)-1;

    player->clearFlag(P_AFK);
    chanStr = cmnd->myCommand->getName();
    i = strlen(cmnd->str[1]);

    // these require special attention - we most provide an
    // override for the check below
    if(chanStr == "dmcls" || chanStr == "dmclass") {

        chanStr = "classsend";
        skip = 2;

        for(check=1; check < max_class; check++)
            if(!strncasecmp(get_class_string(check), cmnd->str[1], i))
                break;

        // these checks are overkill, but it never hurts to be safe:
        // this will force them to use their own class
        if(check == static_cast<int>(CreatureClass::DUNGEONMASTER) && !player->isDm())
            check = 0;
        if(check == static_cast<int>(CreatureClass::CARETAKER) && !player->isCt())
            check = 0;

    } else if(chanStr == "dmrace") {

        chanStr = "racesend";
        skip = 2;

        for(check=1; check<gConfig->raceCount()-1; check++)
            if(!strncasecmp(gConfig->getRace(check)->getName().c_str(), cmnd->str[1], i))
                break;

    } else if(chanStr == "dmclan") {

        chanStr = "clansend";
        skip = 2;

        ClanMap::iterator it;
        Clan *clan=nullptr;

        for(it = gConfig->clans.begin() ; it != gConfig->clans.end() ; it++) {
            clan = (*it).second;
            if(!strncasecmp(cmnd->str[1], clan->getName().c_str(), i)) {
                check = clan->getId();
                break;
            }
        }

    } if(chanStr == "dmguild") {

        skip = 2;
        guild = gConfig->getGuild(player, cmnd->str[1]);

        if(!guild)
            return(0);
    }



    text = getFullstrText(cmnd->fullstr, skip);
    if(text.empty()) {
        player->print("Send what?\n");
        return(0);
    }
    text = confusionText(player, text);


    // this isnt a channel in the list
    if(chanStr == "dmguild") {
        doGuildSend(guild, player, text);
        return(0);
    }


    // get us a channel to use!
    channelPtr chan = getChannelByName(player, chanStr);

    if(!chan)
        return(cmdNoExist(player, cmnd));


    // get us that extra text we'll be attaching to the front of the message
    if(chan->type == COM_CLASS) {

        if(!check)
            check = static_cast<int>(player->getClass());
        extra = "(";
        extra += get_class_string(check);
        if(player->getDeity() && gConfig->classes[get_class_string(check)]->needsDeity()) {
            extra += ":";
            extra += gConfig->getDeity(player->getDeity())->getName();
        }
        extra += ") ";

    } else if(chan->type == COM_RACE) {

        if(!check)
            check = player->getDisplayRace();
        extra = "(";
        extra += gConfig->getRace(check)->getName();
        extra += ") ";

    } else if(chan->type == COM_CLAN) {

        if(!player->getClan() && !player->getDeity() && !check) {
            player->print("You do not belong to a clan or religion.\n");
            return(0);
        }

        extra = "(";
        if(!check) {
            if(player->getDeity()) {
                check = player->getDeityClan();
                extra += gConfig->getDeity(player->getDeity())->getName();
            } else {
                check = player->getClan();
                extra += gConfig->getClan(player->getClan())->getName();
            }
        } else
            extra += gConfig->getClan(check)->getName();
        extra += ") ";

    }


    // it is up to the canUse function to print out the error message
    if(chan->canUse && !chan->canUse(player))
        return(0);
    if(chan->maxLevel != -1 && player->getLevel() > chan->maxLevel && !player->isWatcher()) {
        player->print("You are too high of a level to use that channel.\n");
        return(0);
    }
    if(chan->minLevel != -1 && player->getLevel() < chan->minLevel && !player->isWatcher()) {
        player->print("You must be level %d to use the %s channel.\n", chan->minLevel, chan->channelName);
        return(0);
    }



    if(player->flagIsSet(P_GLOBAL_GAG) && !player->isStaff()) {
        // global gag - don't let them know they're gagged!
        if(!extra.empty())
            player->printColor("%s%s", player->customColorize(chan->color).c_str(), extra.c_str());

        player->printColor(player->customColorize(chan->color + chan->displayFmt).c_str(), player.get(), text.c_str());
    } else {

        std::ostringstream eaves;
        eaves << "--- " << extra << player->getName() << " ";
        if(chan->type == COM_CLASS)
            eaves << "class";
        else if(chan->type == COM_RACE)
            eaves << "race";
        else if(chan->type == COM_CLAN)
            eaves << "clan";
        eaves << " sent, \"" << text << "\".\n";

        std::string etxt = eaves.str();
        etxt = escapeColor(etxt);
        text = escapeColor(text);

        if(chan->discordWebhookID != -1) {
            gServer->sendDiscordWebhook(chan->discordWebhookID, chan->type, player->getName(), text);
        }
        sendGlobalComm(player, text, extra, check, chan, etxt, player->getName(), player->getName());
    }

    return(0);
}

channelPtr getChannelByName(const std::shared_ptr<Player>& player, const std::string &chanStr) {
    for(auto& curChan : channelList) {
        if (curChan.channelName == nullptr) return nullptr;
        if (chanStr == curChan.channelName && (!curChan.canSee || curChan.canSee(player)))
            return &curChan;
    }
    return nullptr;
}
channelPtr getChannelByDiscordChannel(const unsigned long discordChannelID) {
    for(auto& curChan : channelList) {
        if (curChan.channelName == nullptr) return nullptr;
        if (curChan.discordChannelID == -1) continue;
        if (curChan.discordChannelID == discordChannelID) return &curChan;
    }
    return nullptr;
}

void sendGlobalComm(const std::shared_ptr<Player> player, const std::string &text, const std::string &extra, int check,
                    const channelInfo *chan, const std::string &etxt, const std::string &oocName, const std::string &icName) {
    // more complicated checks go here
    std::shared_ptr<Socket> sock=nullptr;
    for(const auto& [pId, ply] : gServer->players) {
        sock = ply->getSock();
        if(!sock->isConnected())
            continue;

        // no gagging staff!
        if(player && ply->isGagging(player->getName()) && !player->isCt())
            continue;
        // deaf people can always hear staff and themselves
        if(ply->isEffected("deafness") && player && !player->isStaff() && ply != player)
            continue;

        // must satisfy all the basic canHear rules to hear this channel
        if( (   (!chan->canHear || chan->canHear(sock)) &&
                (!chan->flag || ply->flagIsSet(chan->flag)) &&
                (!chan->not_flag || !ply->flagIsSet(chan->not_flag)) )
            && ( // they must also satisfy any special conditions here
                (chan->type != COM_CLASS || static_cast<int>(ply->getClass()) == check) &&
                (chan->type != COM_RACE || ply->getDisplayRace() == check) &&
                (chan->type != COM_CLAN || (ply->getDeity() ? ply->getDeityClan() : ply->getClan()) == check) ) )
        {
            if(!extra.empty())
                *ply << ColorOn << ply->customColorize(chan->color) << extra << ColorOff;

            std::string toPrint = chan->displayFmt;
            std::string prompt;

            if(boost::icontains(ply->getSock()->getTermType(), "MUDLET"))
                prompt = " PROMPT";

            std::string oocNameRep;
            std::string icNameRep;

            if(player) {
                icNameRep = mxpTag(std::string("player name='") + player->getName() + "'" + prompt) + icName + mxpTag("/player");
                oocNameRep = mxpTag(std::string("player name='") + player->getName() + "'" + prompt) + oocName + mxpTag("/player");
            } else {
                icNameRep = icName;
                oocNameRep = oocName;
            }
            boost::replace_all(toPrint, "*IC-NAME*", icNameRep);
            boost::replace_all(toPrint, "*OOC-NAME*", oocNameRep);

            if(ply->isStaff() || !player
                || (player->current_language && ply->isEffected("comprehend-languages"))
                || ply->languageIsKnown(player->current_language))
            {
                // Listern speaks this language
                boost::replace_all(toPrint, "*TEXT*", text);
            } else {
                // Listern doesn't speak this language
                boost::replace_all(toPrint, "*TEXT*", "<something incomprehensible>");
            }


            if(player && player->current_language != LCOMMON)
                toPrint += std::string(" in ") + get_language_adj(player->current_language) + ".";

            *ply << ColorOn << ply->customColorize(chan->color) << toPrint << "\n" << ColorOff;
        }


        // even if they fail the check, it might still show up on eaves
        if( chan->eaves && watchingEaves(sock) &&
            !(chan->type == COM_CLASS && check == static_cast<int>(CreatureClass::DUNGEONMASTER))) {
            ply->printColor("^E%s", etxt.c_str());
        }
    }
}


//*********************************************************************
//                      cmdSpeak
//*********************************************************************

int cmdSpeak(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int     lang=0;

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num < 2 ) {
        player->print("Speak what?\n");
        return(0);
    }

    lowercize(cmnd->str[1],0);

    switch (cmnd->str[1][0]) {

    case 'a':
        switch (cmnd->str[1][1]) {
        case 'b':
            lang = LABYSSAL;
            break;
        case 'l':
            lang = 0;
            break;
        case 'r':
            lang = LARCANIC;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 'b':
        lang = LBARBARIAN;
        break;

    case 'c':
        switch (cmnd->str[1][1]) {
        case 'a':
            lang = LINFERNAL;
            break;
        case 'e':
            lang = LCELESTIAL;
            break;
        case 'o':
            lang = LCOMMON;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 'd':
        switch (cmnd->str[1][1]) {
        case 'a':
            lang = LDARKELVEN;
            break;
        case 'r':
            lang = LDRUIDIC;
            break;
        case 'w':
            lang = LDWARVEN;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 'e':
        lang = LELVEN;
        break;
    case 'g':
        switch (cmnd->str[1][1]) {
        case 'i':
            lang = LGIANTKIN;
            break;
        case 'n':
            lang = LGNOMISH;
            break;
        case 'o':
            lang = LGOBLINOID;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 'h':
        switch (cmnd->str[1][1]) {
        case 'u':
            lang = LCOMMON;
            break;
        default:
            switch (cmnd->str[1][4]) {
            case 'e':
                player->print("Half elves have no language of their own.\n");
                player->print("They speak the elven or common tongues.\n");
                return(0);
                break;
            case 'o':
                player->print("Half orcs have no language of their own.\n");
                player->print("They speak the orcish or common tongues.\n");
                return(0);
                break;
            case 'l':
                lang = LHALFLING;
                break;
            case 'g':
                lang = LGIANTKIN;
                break;
            default:
                player->print("You do not know that language.\n");
                return(0);
                break;
            }
            break;
        }
        break;
    case 'i':
        lang = LINFERNAL;
        break;
    case 'k':
        switch (cmnd->str[1][1]) {
        case 'a':
            lang = LKATARAN;
            break;
        case 'e':
            lang = LKENKU;
            break;
        case 'o':
            lang = LKOBOLD;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 'm':
        lang = LMINOTAUR;
        break;
    case 'o':
        switch (cmnd->str[1][1]) {
        case 'g':
            lang = LOGRISH;
            break;
        case 'r':
            lang = LORCISH;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 's':
        switch (cmnd->str[1][1]) {
        case 'e':
            lang = LCELESTIAL;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 't':
        switch (cmnd->str[1][1]) {
        case 'i':
            lang = LTIEFLING;
            break;
        case 'r':
            lang = LTROLL;
            break;
        case 'h':
            lang = LTHIEFCANT;
            break;
        default:
            player->print("You do not know that language.\n");
            return(0);
            break;
        }
        break;
    case 'w':
        lang = LWOLFEN;
        break;
    default:
        player->print("You do not know that language.\n");
        return(0);
        break;
    }


    if(lang == player->current_language) {
        player->print("You're already speaking %s!\n", get_language_adj(lang));
        return(0);
    }

    if(!player->isEffected("tongues") && !player->languageIsKnown(LUNKNOWN+lang) && !player->isStaff()) {
        player->print("You do not know how to speak %s.\n", get_language_adj(lang));
        return(0);
    } else {
        player->print("You will now speak in %s.\n", get_language_adj(lang));
        player->current_language = lang;
    }

    return(0);
}



//*********************************************************************
//                      cmdLanguages
//*********************************************************************

int cmdLanguages(const std::shared_ptr<Player>& player, cmd* cmnd) {
    char    str[2048];
    //  char    lang[LANGUAGE_COUNT][32];
    int     i, j=0;

    player->print("Currently speaking: %s.\n", get_language_adj(player->current_language));
    player->print("Languages known:");

    strcpy(str," ");
    for(i = 0; i < LANGUAGE_COUNT ; i++) {
        if(player->languageIsKnown(LUNKNOWN + i)) {
            j++;
            strcat(str, get_language_adj(i));
            strcat(str, ", ");
        }
    }

    if(!j)
        strcat(str, "None.");
    else {
        str[strlen(str) - 2] = '.';
        str[strlen(str) - 1] = 0;
    }

    player->print("%s\n", str);

    if(player->isEffected("tongues"))
        player->printColor("^yYou have the ability to speak any language.\n");

    if(player->isEffected("comprehend-languages"))
        player->printColor("^yYou have the ability to comprehend any language.\n");

    return(0);
}


//*********************************************************************
//                      printForeignTongueMsg
//*********************************************************************

void printForeignTongueMsg(const std::shared_ptr<const BaseRoom> &inRoom, const std::shared_ptr<Creature>&talker) {
    int         lang=0;

    if(!talker || !inRoom)
        return;

    lang = talker->current_language;
    if(!lang)
        return;

    for(const auto& pIt: inRoom->players) {
        if(auto ply = pIt.lock()) {
            if (ply->languageIsKnown(lang) || ply->isEffected("comprehend-languages") || ply->isStaff()) {
                continue;
            }
            ply->print("%M says something in %s.\n", talker.get(), get_language_adj(lang));
        }
    }
}

//*********************************************************************
//                      sayTo
//*********************************************************************

void Monster::sayTo(const std::shared_ptr<Player>& player, const std::string& message) {
    short language = player->current_language;

    broadcast_rom_LangWc(language, player->getSock(), player->currentLocation, "%M says to %N, \"%s\"^x", this, player.get(), message.c_str());
    printForeignTongueMsg(player->getConstRoomParent(), Containable::downcasted_shared_from_this<Monster>());

    player->printColor("%s%M says to you, \"%s\"\n^x",get_lang_color(language), this, message.c_str());
}


//*********************************************************************
//                      canCommunicate
//*********************************************************************

bool canCommunicate(const std::shared_ptr<Player>& player) {
    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You are not allowed to broadcast.\n");
        return(false);
    }

    // Non staff checks on global communication
    if(!player->isStaff()) {
        if(!player->ableToDoCommand())
            return(false);
        if(player->inJail()) {
            player->print("Jailbirds cannot do that.\n");
            return(false);
        }
        if(player->flagIsSet(P_CANT_BROADCAST)) {
            player->print("Due to abuse, your ability to broadcast is currently disabled.\n");
            return(false);
        }
        if(player->flagIsSet(P_OUTLAW)) {
            player->print("Outlaws cannot do that.\n");
            return(false);
        }
        if(!player->canSpeak()) {
            player->printColor("^yYour voice is too weak to do that.\n");
            return(false);
        }
        // spam check
        if(player->checkForSpam())
            return(false);

    }
    return(true);
}



//*********************************************************************
//                      std::string list functions
//*********************************************************************

int listWrapper(const std::shared_ptr<Player>& player, cmd* cmnd, const char* gerund, const char* noun, std::string (Player::*show)() const,
                bool (Player::*is)(const std::string &name) const, void (Player::*del)(const std::string &name),
                void (Player::*add)(const std::string &name), void (Player::*clear)()) {
    std::shared_ptr<Player> target=nullptr;
    bool online=true;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num == 1) {
        player->print("You are %s: %s\n", gerund, (player.get()->*show)().c_str());
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if((player.get()->*is)(cmnd->str[1])) {
        (player.get()->*del)(cmnd->str[1]);
        player->print("%s removed from your %s list.\n", cmnd->str[1], noun);
        return(0);
    }

    if(cmnd->num == 2) {
        if(!strcmp(cmnd->str[1], "clear")) {
            (player.get()->*clear)();
            player->print("Cleared.\n");
            return(0);
        }
    }

    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        loadPlayer(cmnd->str[1], target);
        online = false;
    }

    if(!target || player == target) {
        player->print("That player does not exist.\n");
    } else if(target->isStaff() && target->getClass() > player->getClass()) {
        player->print("You cannot %s that player.\n", noun);
        if(!online) target.reset();
    } else {

        if(!strcmp(noun, "watch")) {
            if( !player->inSameGroup(target) &&
                !player->isCt() &&
                !(player->isWatcher() && target->getLevel() <= 4)
            ) {
                player->print("%M is not a member of your group.\n", target.get());
                if(!online) target.reset();
                return(0);
            }
        }

        (player.get()->*add)(cmnd->str[1]);
        player->print("%s added to your %s list.\n", target->getCName(), noun);
        if(!online)
            target.reset();
    }

    return(0);
}

int cmdIgnore(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "ignoring", "ignore",
        &Player::showIgnoring,
        &Player::isIgnoring,
        &Player::delIgnoring,
        &Player::addIgnoring,
        &Player::clearIgnoring
    ));
}
int cmdGag(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "gaging", "gag",
        &Player::showGagging,
        &Player::isGagging,
        &Player::delGagging,
        &Player::addGagging,
        &Player::clearGagging
    ));
}
int cmdRefuse(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "refusing", "refuse",
        &Player::showRefusing,
        &Player::isRefusing,
        &Player::delRefusing,
        &Player::addRefusing,
        &Player::clearRefusing
    ));
}
int cmdWatch(const std::shared_ptr<Player>& player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "watching", "watch",
        &Player::showWatching,
        &Player::isWatching,
        &Player::delWatching,
        &Player::addWatching,
        &Player::clearWatching
    ));
}

//*********************************************************************
//                      dmGag
//*********************************************************************

int dmGag(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target;

    if(!player->isStaff() && !player->isWatcher())
        return(cmdNoExist(player, cmnd));
    if(!player->isWatcher())
        return(cmdNoAuth(player));

    if(cmnd->num < 2) {
        player->print("\nGlobally gag whom?\n");
        return(PROMPT);
    }

    lowercize(cmnd->str[1], 1);
    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        player->print("%s is not on.\n", cmnd->str[1]);
        return(0);
    }

    if(target->flagIsSet(P_GLOBAL_GAG)) {
        player->print("%s is no longer globally gagged.\n", target->getCName());
        broadcast(isWatcher, "^C*** %s is no longer globally gagged.", target->getCName());
        broadcast(isDm, "^g*** %s turned off %s's global gag.", player->getCName(), target->getCName());
        target->clearFlag(P_GLOBAL_GAG);
        return(0);
    }

    if(target->isCt()) {
        target->printColor("^R%s tried to gag you!\n", player->getCName());
        player->print("You can't globally gag a DM or CT.\n");
        return(0);
    }

    logn("log.gag", "%s was globally gagged by %s.\n", target->getCName(), player->getCName());
    target->setFlag(P_GLOBAL_GAG);

    broadcast(isWatcher, "^C*** %s has been globally gagged.", target->getCName());
    broadcast(isDm, "^g*** %s globally gagged %s.", player->getCName(), target->getCName());
    return(0);
}

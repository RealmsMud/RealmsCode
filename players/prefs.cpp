/*
 * prefs.cpp
 *   Player preferences
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "boost/format.hpp"

#include "commands.hpp"
#include "creatures.hpp"
#include "login.hpp"
#include "mud.hpp"
#include "rooms.hpp"
#include "server.hpp"
#include "socket.hpp"

// having a pref that starts with a hyphen (-) is instead a category
typedef struct prefInfo {
    bstring name;
    int     flag;
    bool    (*canUse)(const Creature *);
    bstring desc;
    bool    invert;                 // reverses the language
} prefInfo, *prefPtr;

prefInfo prefList[] =
{
    // name         flag                    canUse  desc
    { "-Staff Preferences", 0, isStaff, "", 0 },
    { "eaves",      P_EAVESDROPPER,         isCt,   "eavesdropper mode",        0 },
    { "supereaves", P_SUPER_EAVESDROPPER,   isCt,   "super eavesdropper mode",  0 },
    { "msg",        P_NO_MSG,               isCt,   "*msg channel",             true },
    { "mobtick",    P_NO_TICK_MSG,          isCt,   "See mob tick broadcasts",  true },
    { "mobaggro",   P_NO_AGGRO_MSG,         isCt,   "See mob aggro broadcasts", true },
    { "mobdeath",   P_NO_DEATH_MSG,         isCt,   "See mob death broadcasts", true },
    { "ttobound",   P_T_TO_BOUND,           isStaff,"*t to bound room",         0 },
    { "wts",        P_NO_WTS,               isCt,   "*wts channel",             true },
    { "zones",      P_VIEW_ZONES,           isCt,   "view overland zones",      0 },
    { "autoinvis",  P_AUTO_INVIS,           isCt,   "auto-invis if idle for 100+ mins",     0 },
    { "allhooks",   P_SEE_ALL_HOOKS,        isDm,   "see all hooks",            0 },
    { "hooks",      P_SEE_HOOKS,            isDm,   "see triggered hooks",      0 },

    { "-Color",     0, 0, "", 0 },
    { "mirc",       P_MIRC,                 0,      "mirc colors",              0 },
    { "ansi",       P_ANSI_COLOR,           0,      "ansi colors",              0 },
    { "langcolor",  P_LANGUAGE_COLORS,      0,      "language colors",          0 },
    { "extracolor", P_NO_EXTRA_COLOR,       0,      "extra color options",      true },
    { "areacolor",  P_INVERT_AREA_COLOR,    0,      "invert area colors",       0 },
    { "mxpcolors",  P_MXP_ENABLED,          0,      "use MXP for more than 16 colors",      0 },

    { "-Channels",  0, 0, "", 0 },
    { "cls",        P_IGNORE_CLASS_SEND,    0,      "class channel",            true },
    { "race",       P_IGNORE_RACE_SEND,     0,      "race channel",             true },
    { "broad",      P_NO_BROADCASTS,        0,      "broadcast channel",        true },
    { "newbie",     P_IGNORE_NEWBIE_SEND,   0,      "newbie channel",           true },
    { "gossip",     P_IGNORE_GOSSIP,        0,      "gossip channel",           true },
    { "clan",       P_IGNORE_CLAN,          0,      "clan channel",             true },
    { "tells",      P_NO_TELLS,             0,      "send/tell/whisper/sign",   true },
    { "sms",        P_IGNORE_SMS,           0,      "receive text messages",    true },
    { "ignore",     0,                      0,      "ignore all channels",      0 },
    // do something with P_IGNORE_ALL

    { "-Notifications", 0, 0, "", 0 },
    { "duel",       P_NO_DUEL_MESSAGES,     0,      "duel messages",            true },
    { "login",      P_NO_LOGIN_MESSAGES,    0,      "login messages",           true },
    { "shopprofit", P_DONT_SHOW_SHOP_PROFITS,0,     "shop profit notifications",true },
    { "mail",       P_NO_SHOW_MAIL,         0,      "mudmail notifications",    true },
    { "permdeath",  P_PERM_DEATH,           0,      "perm death broadcasts",    0 },
    { "auction",    P_NO_AUCTIONS,          0,      "player auctions",          true },
    { "showforum",  P_HIDE_FORUM_POSTS,     0,      "notification of forum posts", true },
    { "forumbrevity",P_FULL_FORUM_POSTS,    0,      "forum posts notification length", true },
    { "notifications",0,                    0,      "show all notifications",   0 },

    { "-Combat",    0, 0, "", 0 },
    { "autoattack", P_NO_AUTO_ATTACK,       0,      "auto attack",              true },
    { "lagprotect", P_LAG_PROTECTION_SET,   0,      "lag protection",           0 },
    { "lagrecall",  P_NO_LAG_HAZY,          0,      "recall potion if below half hp",true },
    { "wimpy",      P_WIMPY,                0,      "flee when HP below this number",0 },
    { "killaggros", P_KILL_AGGROS,          0,      "attack aggros first",      0 },
    { "mobnums",    P_NO_NUMBERS,           0,      "monster ordinal numbers",  true },
    { "autotarget", P_NO_AUTO_TARGET,       0,      "automatic targeting",      true },

    { "-Group",     0, 0, "", 0 },
    { "group",      P_IGNORE_GROUP_BROADCAST,0,     "group combat messages",    true },
    { "xpsplit",    P_XP_DIVIDE,            0,      "group experience split",   0 },
    { "split",      P_GOLD_SPLIT,           0,      "split gold among group",   0 },
    { "stats",      P_NO_SHOW_STATS,        0,      "show group your stats",    true },
    { "follow",     P_NO_FOLLOW,            0,      "can be followed",          true },

    { "-Display",   0, 0, "", 0 },
    { "showall",    P_SHOW_ALL_PREFS,       0,      "show all preferences",     false },
    { "short",      P_NO_SHORT_DESCRIPTION, 0,      "short description",        true },
    { "long",       P_NO_LONG_DESCRIPTION,  0,      "long description",         true },
    { "nlprompt",   P_NEWLINE_AFTER_PROMPT, 0,      "newline after prompt",     0 },
    { "xpprompt",   P_SHOW_XP_IN_PROMPT,    0,      "show exp in prompt",       0 },
    { "prompt",     P_PROMPT,               0,      "descriptive prompt",       0 },
    { "compact",    P_COMPACT,              0,      "compacts most output",     false },
    { "tick",       P_SHOW_TICK,            0,      "shows ticks",              false },
    { "skillprogress", P_SHOW_SKILL_PROGRESS,0,     "show skill progress bar",  false },

    { "-Miscellaneous", 0, 0, "", 0 },
    { "autowear",   P_NO_AUTO_WEAR,         0,      "wear all on login",        true },
    { "summon",     P_NO_SUMMON,            0,      "can be summoned",          true },
    //{ "pkill",        P_NO_PKILL_PERCENT,     0,      "show pkill percentage",    true },
    { "afk",        P_AFK,                  0,      "away from keyboard",       0 },
    { "statistics", P_NO_TRACK_STATS,       0,      "track statistics",         true },

    // handle wimpy below
    // handle afk below

    { "", 0, 0, "" }
};

//*********************************************************************
//                      cmdTelOpts
//*********************************************************************

int cmdTelOpts(Player* player, cmd* cmnd) {
    Socket* sock = player->getSock();
    Player* target = player;


    if(!sock)
        return(0);
    if(cmnd->num > 1) {
        if(!strcasecmp(cmnd->str[1], "wrap")) {
            player->print("Setting wrap to '%d'\n", player->setWrap(cmnd->val[1]));
            return(0);
        } else {
            if(player->isCt()) {
                cmnd->str[1][0] = up(cmnd->str[1][0]);
                target = gServer->findPlayer(cmnd->str[1]);
                if(target && (!target->isDm() || player->isDm())) {
                    sock = target->getSock();
                    if(!sock) {
                        player->print("No socket.\n");
                        return(0);
                    }
                } else {
                    player->print("Unknown option.\n");
                    return(0);
                }
            } else {
                player->print("Unknown option.\n");
                return(0);
            }
        }
    }

    std::ostringstream oStr;
    boost::format formatWithDesc("%1% (%2%) %|40t|%3%\n");
    boost::format formatNoDesc("%1% ^c%|42t|%2%^x\n");
    formatNoDesc.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )  );
    formatWithDesc.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )  );
    try {
        if(target != player)
            oStr << "^cNegotiated Telnet Options for " << target->getName() << "^x\n";
        else
            oStr << "^cNegotiated Telnet Options^x\n";

        oStr << "-------------------------------------------------------\n";
        oStr << formatNoDesc % "Term" % sock->getTermType();
        oStr << formatNoDesc % "Term Size" % bstring(bstring(sock->getTermCols()) + " x " + bstring(sock->getTermRows()));
        if(player->getWrap() == 0)
            oStr << formatNoDesc % "Server Linewrap" % "None";
        else if(player->getWrap() == -1)
            oStr << formatNoDesc % "Server Linewrap" % (bstring("Using Cols (") + sock->getTermCols() + bstring(")"));
        else
            oStr << formatNoDesc % "Server Linewrap" % target->getWrap();
        oStr << formatWithDesc % "MCCP" % "Mud Client Compression Protocol" % ((sock->getMccp() != 0) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "MXP" % "MUD eXtension Protocol" % ((sock->getMxp() == true) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "MSDP" % "MUD Server Data Protocol" % ((sock->getMsdp() == true) ? "^gon^x" : "^roff^x");
        if (sock->getMsdp()) {
            oStr << "\t MSDP Reporting: " << sock->getMsdpReporting() << "\n";

        }
        oStr << formatWithDesc % "ATCP" % "Achaea Telnet Client Protocol" % ((sock->getAtcp() == true) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "Charset" % "Charset Negotiation" % ((sock->getCharset() == true) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "UTF-8" % "UTF-8 Support" % ((sock->getUtf8() == true) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "MSP" % "Mud Sound Protocol" % ((sock->getMsp() == true) ? "^gon^x" : "^roff^x");
        oStr << formatWithDesc % "EOR" % "End of Record" % ((sock->getEor() == true) ? "^gon^x" : "^roff^x");
    } catch (std::exception &e) {
        std::clog << e.what() << std::endl;
    }

    player->printColor("%s", oStr.str().c_str());
    //player->print("  %-13s ::  %-31s ::  ", prefList[i].name.c_str(), prefList[i].desc.c_str());
    return(0);
}

//*********************************************************************
//                      cmdPrefs
//*********************************************************************

int cmdPrefs(Player* player, cmd* cmnd) {
    prefPtr pref = nullptr;
    int     i=0, match=0, len=strlen(cmnd->str[1]);
    bool    set = cmnd->str[0][0]=='s';
    bool    toggle = cmnd->str[0][0]=='t' || cmnd->str[0][0]=='p';
    bool    show=false;
    bool all = !strcmp(cmnd->str[1], "-all") || (len == 0 && player->flagIsSet(P_SHOW_ALL_PREFS));
    bstring prefName="";

    if(!player->ableToDoCommand())
        return(0);

    // display a list of preferences
    if(cmnd->num == 1 || cmnd->str[1][0] == '-') {

        if(!len || !all) {
            player->print("Below is a list of preference categories. To view a cateogry, type\n");
            player->print("\"pref -[category]\". To view all preferences, type \"pref -all\".\n\n");
        }

        while(prefList[i].name != "") {
            if(!prefList[i].canUse || prefList[i].canUse(player)) {

                if(prefList[i].name.at(0) == '-') {
                    // previous category was shown? newline
                    if(show)
                        player->print("\n");
                    // no parameters? indent
                    if(!len)
                        player->print("  ");

                    prefName = prefList[i].name;
                    show = all || (len && !strncmp(cmnd->str[1], prefName.toLower().c_str(), len));

                    player->printColor("^%s%s^w\n", show ? "C" : "c", prefList[i].name.substr(1, prefList[i].name.length() - 1).c_str());
                } else {
                    if(!show) {
                        i++;
                        continue;
                    }
                    player->print("  %-13s ::  %-31s ::  ", prefList[i].name.c_str(), prefList[i].desc.c_str());
                    // wimpy gets its own special format
                    if(prefList[i].name == "wimpy" && player->flagIsSet(prefList[i].flag))
                        player->print("%d", player->getWimpy());
                    else if(prefList[i].name == "forumbrevity")
                        player->print("%s", player->flagIsSet(prefList[i].flag) ? "full" : "brief");
                    // print everything exp ignore
                    else if(prefList[i].flag)
                        player->printColor("%s^w", player->flagIsSet(prefList[i].flag) ^ !prefList[i].invert ? "^roff" : "^gon");
                    player->print("\n");
                }

            }
            i++;
        }
        player->print("\n");
        return(0);
    }


    // everyone is used to nosummon - let them keep it that way
    if(!strcmp(cmnd->str[1], "nosummon")) {
        if(!toggle)
            set = !set;
        strcpy(cmnd->str[1], "summon");
    }


    // we're setting or clearing a preference
    // find out which one
    while(prefList[i].name != "") {
        if( prefList[i].name.at(0) != '-' &&
            (!prefList[i].canUse || prefList[i].canUse(player))
        ) {

            if(!strcmp(cmnd->str[1], prefList[i].name.c_str())) {
                match = 1;
                pref = &prefList[i];
                break;
            } else if(!strncmp(cmnd->str[1], prefList[i].name.c_str(), len)) {
                match++;
                pref = &prefList[i];
            }

        }
        i++;
    }

    if(match > 1) {
        player->print("Preference not unique.\n");
        return(0);
    }
    if(!pref) {
        player->print("Unknown preference.\n");
        return(0);
    }


    // if any channels are being ignored, toggle = stop ignorning
    if(toggle && pref->name == "ignore") {
        toggle = false;
        set = (!player->flagIsSet(P_IGNORE_CLASS_SEND) &&
            !player->flagIsSet(P_IGNORE_RACE_SEND) &&
            !player->flagIsSet(P_NO_BROADCASTS) &&
            !player->flagIsSet(P_IGNORE_NEWBIE_SEND) &&
            !player->flagIsSet(P_IGNORE_GOSSIP) &&
            !player->flagIsSet(P_IGNORE_CLAN) &&
            !player->flagIsSet(P_NO_TELLS)
        );
    }

    // if any notifications are off, toggle = turn them all on
    if(toggle && pref->name == "notifications") {
        toggle = false;
        set = !(player->flagIsSet(P_NO_DUEL_MESSAGES) ||
            !player->flagIsSet(P_NO_LOGIN_MESSAGES) ||
            !player->flagIsSet(P_DONT_SHOW_SHOP_PROFITS) ||
            !player->flagIsSet(P_NO_SHOW_MAIL) ||
            !player->flagIsSet(P_NO_SHOW_MAIL) ||
            !player->flagIsSet(P_IGNORE_GROUP_BROADCAST) ||
            player->flagIsSet(P_PERM_DEATH) ||
            !player->flagIsSet(P_NO_AUCTIONS) ||
            !player->flagIsSet(P_HIDE_FORUM_POSTS)
        );
    }

    // set
    //---------------

    //NORMAL
    // toggle
    // !flag

    // toggle
    // invert && flag

    // are we setting or clearing?
    if( ((set ^ pref->invert) && !toggle) ||
        (toggle && !player->flagIsSet(pref->flag))
    ) {

        if(pref->flag == P_MXP_ENABLED) {
            if(!player->getSock()->getMxp()) {
                player->print("Your client does not support MXP!\n");
                return(0);
            }
            player->getSock()->defineMxp();
        }

        if(pref->name == "ansi") {
            player->getSock()->setColorOpt(ANSI_COLOR);
        }

        if(pref->name == "ignore") {
            player->setFlag(P_IGNORE_CLASS_SEND);
            player->setFlag(P_IGNORE_RACE_SEND);
            player->setFlag(P_NO_BROADCASTS);
            player->setFlag(P_IGNORE_NEWBIE_SEND);
            player->setFlag(P_IGNORE_GOSSIP);
            player->setFlag(P_IGNORE_CLAN);
            player->setFlag(P_NO_TELLS);
        } else if(pref->name == "notifications") {
            player->clearFlag(P_NO_DUEL_MESSAGES);
            player->clearFlag(P_NO_LOGIN_MESSAGES);
            player->clearFlag(P_DONT_SHOW_SHOP_PROFITS);
            player->clearFlag(P_NO_SHOW_MAIL);
            player->clearFlag(P_IGNORE_GROUP_BROADCAST);
            player->setFlag(P_PERM_DEATH);
            player->clearFlag(P_NO_AUCTIONS);
            player->clearFlag(P_HIDE_FORUM_POSTS);
        } else
            player->setFlag(pref->flag);

        if(pref->name == "afk") {
            // broadcast for going afk
            if(!player->flagIsSet(P_DM_INVIS))
                broadcast(player->getSock(), player->getParent(), "%M has gone afk.", player);
            else
                broadcast(isCt, player->getSock(), player->getRoomParent(), "*DM* %M has gone afk.", player);
        }
        if(pref->name == "wimpy") {
            player->setWimpy(cmnd->val[1] == 1L ? 10 : cmnd->val[1]);
            player->setWimpy(MAX(player->getWimpy(), 2));
        }
        set = true;

    } else {
        if(pref->name == "ansi") {
            player->getSock()->setColorOpt(NO_COLOR);
        }

        if(pref->name == "ignore") {
            player->clearFlag(P_IGNORE_CLASS_SEND);
            player->clearFlag(P_IGNORE_RACE_SEND);
            player->clearFlag(P_NO_BROADCASTS);
            player->clearFlag(P_IGNORE_NEWBIE_SEND);
            player->clearFlag(P_IGNORE_GOSSIP);
            player->clearFlag(P_IGNORE_CLAN);
            player->clearFlag(P_NO_TELLS);
        } else if(pref->name == "notifications") {
            player->setFlag(P_NO_DUEL_MESSAGES);
            player->setFlag(P_NO_LOGIN_MESSAGES);
            player->setFlag(P_DONT_SHOW_SHOP_PROFITS);
            player->setFlag(P_NO_SHOW_MAIL);
            player->setFlag(P_IGNORE_GROUP_BROADCAST);
            player->clearFlag(P_PERM_DEATH);
            player->setFlag(P_NO_AUCTIONS);
            player->setFlag(P_HIDE_FORUM_POSTS);
        } else
            player->clearFlag(pref->flag);

        if(pref->name == "wimpy")
            player->setWimpy(0);

        set = false;

    }


    // give them the message
    player->print("Preference : %s : ", pref->desc.c_str());
    if(pref->name == "wimpy" && player->flagIsSet(pref->flag))
        player->print("%d", player->getWimpy());
    else if(pref->name == "forumbrevity")
        player->print("%s", player->flagIsSet(pref->flag) ? "full" : "brief");
    else
        player->printColor("%s^w", set ^ !pref->invert ? "^roff" : "^gon");
    player->print(".\n");

    if(pref->flag == P_NO_TRACK_STATS)
        player->print("Note that player kill statistics are always tracked and cannot be reset.\n");

    return(0);
}

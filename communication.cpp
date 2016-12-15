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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "clans.h"
#include "commands.h"
#include "communication.h"
#include "config.h"
#include "creatures.h"
#include "deityData.h"
#include "guilds.h"
#include "mud.h"
#include "move.h"
#include "playerClass.h"
#include "raceData.h"
#include "rooms.h"
#include "server.h"
#include "socket.h"
#include "xml.h"

//*********************************************************************
//                      confusionChar
//*********************************************************************

char confusionChar() {
    int n;
    switch(mrand(0,10)) {
    case 0:
        // random upper case
        return(mrand(65, 90));
    case 1:
    case 2:
        // random character
        n = mrand(0,57);
        // weight all the characters in the different ranges evenly
        if(n <= 31) {
            // mrand(91, 121);      31 chars
            return(n + 91);
        } else if(n <= 38) {
            // mrand(58, 64);       7 chars
            return(n - 31 + 58);
        } else if(n <= 54) {
            // mrand(32, 47);       16 chars
            return(n - 31 - 7 + 32);
        } else {
            // mrand(123, 125);     3 chars
            return(n - 31 - 7 - 16 + 123);
        }
    case 3:
        // random number
        return(mrand(48, 57));
        break;
    default:
        break;
    }
    // random lower case
    return(mrand(97, 122));
}

//*********************************************************************
//                      confusionText
//*********************************************************************
// make the player say gibberish instead of actual words

bstring confusionText(Creature* speaker, bstring text) {
    if( (!speaker->isEffected("confusion") && !speaker->isEffected("drunkenness")) ||
        speaker->isStaff()
    )
        return(text);

    bool scramble = mrand(0,1);

    for(int i=0; i<text.getLength(); i++) {
        if(text.getAt(i) == ' ') {
            scramble = !mrand(0,3);
            // sometimes scramble spaces, too
            if(!mrand(0,8))
                text.setAt(i, confusionChar());
        } else {

            if(scramble || !mrand(0,8))
                text.setAt(i, confusionChar());

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

bstring getFullstrTextTrun(bstring str, int skip, char toSkip, bool colorEscape) {
    return(getFullstrText(str, skip, toSkip, colorEscape, true));
}

bstring getFullstrText(bstring str, int skip, char toSkip, bool colorEscape, bool truncate) {
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

int communicateWith(Player* player, cmd* cmnd) {
    bstring text = "";
    Player  *target=0;
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
    Player* ply;
    for(std::pair<bstring, Player*> p : gServer->players) {
        ply = p.second;

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
                player->print("%M is blind! %s can't see what you're signing.\n", target, target->upHeShe());
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
    if(text == "") {
        player->print("%s what?\n", com_text_u[chan->type]);
        return(0);
    }
    text = confusionText(player, text);

    player->printColor("^cYou %s \"%s%s\" to %N.\n",
        com_text[chan->type],
        ooc_str, text.c_str(), target);


    if(chan->type == COM_WHISPER)
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M whispers something to %N.", player, target);


    if(!target->isGagging(player->getName())) {
        target->printColor("%s%s%M %s%s, \"%s%s\".\n",
            target->customColorize("*CC:TELL*").c_str(),
            chan->type == COM_WHISPER || chan->type == COM_SIGN ? "" : "### ",
            player,
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

        for(Player* ply : player->getRoomParent()->players) {
            if( ply != player && ply != target && !ply->isEffected("blindness"))
            {
                if(ply->getRace() == DARKELF || ply->isStaff())
                    ply->print("%M signed, \"%s\" to %N.\n", player, text.c_str(), target);
                else
                    ply->print("%M signed something in dark elven to %N.\n", player, target);
            }
        }

    }

    if(player->isDm() || target->isDm())
        return(0);

    broadcast(watchingEaves, "^E--- %s %s to %s, \"%s%s\".", player->getCName(),
        com_text[chan->type], target->getCName(), ooc_str, text.c_str());

    player->bug("%s %s, \"%s%s\" to %s.\n", player->getCName(),
        com_text[chan->type], ooc_str, text.c_str(), target->getCName());
    target->bug("%s %s, \"%s%s\" to %s.\n", player->getCName(),
        com_text[chan->type], ooc_str, text.c_str(), target->getCName());

    if(chan->type != COM_SIGN)
        target->setLastCommunicate(player->getName());
    return(0);
}


//*********************************************************************
//                      commTarget
//*********************************************************************
// function that does the actual printing of message for below

void commTarget(Creature* player, Player* target, int type, bool ooc, int lang, bstring text, bstring speak, char *ooc_str, bool anon) {
    std::ostringstream out;

    if(!target || target->flagIsSet(P_UNCONSCIOUS))
        return;
    if(target->isEffected("deafness") && !player->isStaff())
        return;

    if(type != COM_GT)
        speak += "s";

    if(type == COM_EMOTE) {
        out << player->getCrtStr(target, CAP) << " " << text << "\n";
        target->bug("%s emoted: %s.\n", player->getCName(), text.c_str());

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

//      if(!ooc && target->flagIsSet(P_LANGUAGE_COLORS))
//          ANSI(fd, get_lang_color(lang));
        out << ooc_str << text;
        out << "\".\n";

        target->bug("%s %s in %s, \"%s%s.\"\n", player->getCName(), com_text[type],
            get_language_adj(lang), ooc_str, text.c_str());

    } else {
        if(anon) out << "Someone";
        else     out << player->getCrtStr(target, CAP);

        out << " " << speak << " something in " << get_language_adj(lang) << ".\n";
        target->bug("%s %s something in %s.\n", player->getCName(), speak.c_str(), get_language_adj(lang));
    }
    *target << ColorOn << out.str() << ColorOff;
//  target->printColor("%s", out.str().c_str());
}

//*********************************************************************
//                      communicate
//*********************************************************************
// This function allows the player to say something to all the other
// people in the room (or nearby rooms).

int pCommunicate(Player* player, cmd* cmnd) {
    return(communicate(player, cmnd));
}

int communicate(Creature* creature, cmd* cmnd) {
    const Player* player=0;
    bstring text = "", name = "";
    Creature *owner=0;
    Player* pTarget=0;
    BaseRoom* room=0;

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
    if(text == "") {
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
            for(Creature* crt : group->members) {
                pTarget = crt->getAsPlayer();
                if(!pTarget) continue;
                // GT prints to the player!
                if(pTarget == creature && chan->type != COM_GT)
                    continue;

                if(pTarget->isGagging(creature->isPet() ? creature->getMaster()->getCName() : creature->getCName()))
                    continue;

                if(pTarget->getGroupStatus() < GROUP_MEMBER) continue;

                commTarget(creature, pTarget, chan->type, chan->ooc, lang, text, speak, ooc_str, false);
            }
        } else {
            creature->print("You are not in a group.\n");
            return(0);
        }

    } else {
        creature->unhide();

        if(chan->type == COM_EMOTE) {

            creature->printColor("You emote: %s.\n", text.c_str());
            player->bug("%s emoted: %s.\n", creature->getCName(), text.c_str());

        } else {
            char intro[2046];
            if(chan->ooc || lang == LCOMMON)
                sprintf(intro, "You %s,", speak);
            else
                sprintf(intro, "You %s in %s,", speak, get_language_adj(lang));

            creature->printColor("%s%s \"%s%s\".\n^x", ((!chan->ooc && creature->flagIsSet(P_LANGUAGE_COLORS)) ? get_lang_color(lang) : ""),
                    intro, ooc_str, text.c_str());
            player->bug("%s %s in %s, \"%s%s.\"\n", creature->getCName(), com_text[chan->type],
                get_language_adj(lang), ooc_str, text.c_str());

        }

        for(Player* ply : creature->getRoomParent()->players) {
            if(chan->shout)
                ply->wake("Loud noises disturb your sleep.", true);

            // GT prints to the player!
            if(ply == creature && chan->type != COM_GT)
                continue;

            if(ply->isGagging(creature->isPet() ? creature->getMaster()->getName() : creature->getName()))
                continue;

            commTarget(creature, ply, chan->type, chan->ooc, lang, text, speak, ooc_str, false);
        }

        if(chan->shout) {
            // Because of multiple exits leading to the same room, we will keep
            // track of who has heard us shout
            std::list<Socket*> listeners;
            std::list<Socket*>::iterator it;
            bool    heard = false;

            // This allows a player to yell something that will be heard
            // not only in their room, but also in all rooms adjacent to them. In
            // the adjacent rooms, however, people will not know who yelled.


            for(Exit* exit : creature->getRoomParent()->exits) {
                room = 0;
                i=0;
                PlayerSet::iterator pIt, pEnd;
                // don't shout through closed doors
                if(!exit->flagIsSet(X_CLOSED))
                    Move::getRoom(0, exit, &room, true);

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
                    pTarget = (*pIt++);

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

            for(Exit* exit : creature->getRoomParent()->exits) {
                // got the phrase right?
                if(exit->getPassPhrase() != "" && exit->getPassPhrase() == text) {
                    // right language?
                    if(!exit->getPassLanguage() || lang == exit->getPassLanguage()) {
                        // even needs to be open?
                        if(exit->flagIsSet(X_LOCKED)) {
                            broadcast(nullptr, creature->getRoomParent(), "The %s opens!", exit->getCName());
                            exit->clearFlag(X_LOCKED);
                            exit->clearFlag(X_CLOSED);

                            if(exit->getOpen() != "") {
                                if(exit->flagIsSet(X_ONOPEN_PLAYER)) {
                                    creature->print("%s.\n", exit->getOpen().c_str());
                                } else {
                                    broadcast(0, creature->getRoomParent(), exit->getOpen().c_str());
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

    Player* pPlayer = creature->getAsPlayer();
    // spam check
    if(pPlayer)
        pPlayer->checkForSpam();

    return(0);
}

bstring mxpTag(bstring str) {
    return( bstring(MXP_BEG) + str + bstring(MXP_END));
}
//*********************************************************************
//                      channel
//**********************************************************************
// This function is used as a base for all global communication channels

int channel(Player* player, cmd* cmnd) {
    bstring text = "", chanStr = "", extra = "";
    int     i=0, check=0, skip=1;
    const Guild* guild=0;

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

        std::map<int, Clan*>::iterator it;
        Clan *clan=0;

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
    if(text == "") {
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
    channelPtr chan = nullptr;
    i = 0;
    while(channelList[i].channelName != nullptr) {
        if( !strcmp(chanStr.c_str(), channelList[i].channelName) &&
            (!channelList[i].canSee || channelList[i].canSee(player))
        ) {
            chan = &channelList[i];
            break;
        }
        i++;
    }

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
        extra += gConfig->getRace(check)->getName().c_str();
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
        //oldPrint(fd, "You are not authorized to use that channel.\n");
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
        if(extra != "")
            player->printColor("%s%s", player->customColorize(chan->color).c_str(), extra.c_str());

        player->printColor(player->customColorize(chan->color + chan->displayFmt).c_str(), player, text.c_str());
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

        bstring etxt = eaves.str();
        etxt = escapeColor(etxt);
        text = escapeColor(text);

        // more complicated checks go here
        Player* ply=0;
        Socket* sock=0;
        for(std::pair<bstring, Player*> p : gServer->players) {
            ply = p.second;

            sock = ply->getSock();
            if(!sock->isConnected())
                continue;

            // no gagging staff!
            if(player && ply->isGagging(player->getName()) && !player->isCt())
                continue;
            // deaf people can always hear staff and themselves
            if(ply->isEffected("deafness") && !player->isStaff() && ply != player)
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
                if(extra != "")
                    *ply << ColorOn << ply->customColorize(chan->color) << extra << ColorOff;

                bstring toPrint = chan->displayFmt;
                bstring icName = player->getCrtStr(ply, ply->displayFlags() | CAP);
                bstring oocName = player->getName();
                bstring prompt = "";

                if(ply->getSock()->getTermType().toUpper().find("MUDLET") != bstring::npos)
                    prompt = " PROMPT";

                if(ply->canSee(player)) {
                    icName = mxpTag(bstring("player name='") + player->getName() + "'" + prompt ) + icName + mxpTag("/player");
                    oocName = mxpTag(bstring("player name='") + player->getName()  + "'" + prompt) + oocName + mxpTag("/player");
                }

                toPrint.Replace("*IC-NAME*", icName.c_str());
                toPrint.Replace("*OOC-NAME*", oocName.c_str());
                //std::cout << "ToPrint: " << toPrint << std::endl;

                if(ply->isStaff() || (player->current_language && ply->isEffected("comprehend-languages"))
                        || ply->languageIsKnown(player->current_language))
                {
                    // Listern speaks this language
                    toPrint.Replace("*TEXT*", text.c_str());
                } else {
                    // Listern doesn't speak this language
                    toPrint.Replace("*TEXT*", "<something incomprehensible>");
                }


                if(player->current_language != LCOMMON)
                    toPrint += bstring(" in ") + get_language_adj(player->current_language) + ".";

                *ply << ColorOn << ply->customColorize(chan->color) << toPrint << "\n" << ColorOff;
            }


            // even if they fail the check, it might still show up on eaves
            if( chan->eaves &&
                watchingEaves(sock) &&
                !(chan->type == COM_CLASS && check == static_cast<int>(CreatureClass::DUNGEONMASTER))
            ) {
                ply->printColor("^E%s", etxt.c_str());
            }
        }
    }

    return(0);
}


//*********************************************************************
//                      cmdSpeak
//*********************************************************************

int cmdSpeak(Player* player, cmd* cmnd) {
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

int cmdLanguages(Player* player, cmd* cmnd) {
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

void printForeignTongueMsg(const BaseRoom *inRoom, Creature *talker) {
    int         lang=0;

    if(!talker || !inRoom)
        return;

    lang = talker->current_language;
    if(!lang)
        return;

    for(Player* ply : inRoom->players) {
        if(ply->languageIsKnown(lang) || ply->isEffected("comprehend-languages") || ply->isStaff()) {
            continue;
        }
        ply->print("%M says something in %s.\n", talker, get_language_adj(lang));
    }
}

//*********************************************************************
//                      sayTo
//*********************************************************************

void Monster::sayTo(const Player* player, const bstring& message) {
    short language = player->current_language;

    broadcast_rom_LangWc(language, player->getSock(), player->currentLocation,
        "%M says to %N, \"%s\"^x", this, player, message.c_str());
    printForeignTongueMsg(player->getConstRoomParent(), this);

    player->printColor("%s%M says to you, \"%s\"\n^x",get_lang_color(language), this, message.c_str());
}


//*********************************************************************
//                      canCommunicate
//*********************************************************************

bool canCommunicate(Player* player) {
    if(player->getClass() == CreatureClass::BUILDER) {
        player->print("You are not allowed to broadcast.\n");
        return(false);
    }

    // Non staff checks on global communication
    if(!player->isStaff()) {
        if(!player->ableToDoCommand())
            return(false);
        if(player->flagIsSet(P_CANT_BROADCAST)) {
            player->print("Due to abuse, you no longer have that privilage.\n");
            return(false);
        }
        if(player->inJail()) {
            player->print("People in jail do not have that privilage.\n");
            return(false);
        }

        if(player->flagIsSet(P_OUTLAW)) {
            player->print("You're an outlaw, you don't have that privilage.\n");
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
//                      bstring list functions
//*********************************************************************

int listWrapper(Player* player, cmd* cmnd, const char* gerund, const char* noun, bstring (Player::*show)(void) const, bool (Player::*is)(bstring name) const, void (Player::*del)(bstring name), void (Player::*add)(bstring name), void (Player::*clear)(void)) {
    Player  *target=0;
    bool online=true;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(cmnd->num == 1) {
        player->print("You are %s: %s\n", gerund, (player->*show)().c_str());
        return(0);
    }

    cmnd->str[1][0] = up(cmnd->str[1][0]);

    if((player->*is)(cmnd->str[1])) {
        (player->*del)(cmnd->str[1]);
        player->print("%s removed from your %s list.\n", cmnd->str[1], noun);
        return(0);
    }

    if(cmnd->num == 2) {
        if(!strcmp(cmnd->str[1], "clear")) {
            (player->*clear)();
            player->print("Cleared.\n");
            return(0);
        }
    }

    target = gServer->findPlayer(cmnd->str[1]);

    if(!target) {
        loadPlayer(cmnd->str[1], &target);
        online = false;
    }

    if(!target || player == target) {
        player->print("That player does not exist.\n");
    } else if(target->isStaff() && target->getClass() > player->getClass()) {
        player->print("You cannot %s that player.\n", noun);
        if(!online)
            free_crt(target);
    } else {

        if(!strcmp(noun, "watch")) {
            if( !player->inSameGroup(target) &&
                !player->isCt() &&
                !(player->isWatcher() && target->getLevel() <= 4)
            ) {
                player->print("%M is not a member of your group.\n", target);
                if(!online)
                    free_crt(target);
                return(0);
            }
        }

        (player->*add)(cmnd->str[1]);
        player->print("%s added to your %s list.\n", target->getCName(), noun);
        if(!online)
            free_crt(target);
    }

    return(0);
}

int cmdIgnore(Player* player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "ignoring", "ignore",
        &Player::showIgnoring,
        &Player::isIgnoring,
        &Player::delIgnoring,
        &Player::addIgnoring,
        &Player::clearIgnoring
    ));
}
int cmdGag(Player* player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "gaging", "gag",
        &Player::showGagging,
        &Player::isGagging,
        &Player::delGagging,
        &Player::addGagging,
        &Player::clearGagging
    ));
}
int cmdRefuse(Player* player, cmd* cmnd) {
    return(listWrapper(player, cmnd, "refusing", "refuse",
        &Player::showRefusing,
        &Player::isRefusing,
        &Player::delRefusing,
        &Player::addRefusing,
        &Player::clearRefusing
    ));
}
int cmdWatch(Player* player, cmd* cmnd) {
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

int dmGag(Player* player, cmd* cmnd) {
    Player  *target;

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

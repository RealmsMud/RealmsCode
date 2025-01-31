/*
 * singers.cpp
 *   Functions that deal with bard class players
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

#include <cstdio>                    // for sprintf
#include <cstdlib>                   // for qsort
#include <cstring>                   // for strcat, strlen, strncmp, strncpy
#include <ctime>                     // for time
#include <list>                      // for operator==, _List_iterator, list
#include <set>                       // for operator==, _Rb_tree_const_iterator
#include <string>                    // for allocator, operator==, string

#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for cmdCharm, cmdSing, cmdSongs
#include "config.hpp"                // for Config, gConfig
#include "flags.hpp"                 // for P_CHARMED, P_AFK, M_CHARMED, M_N...
#include "global.hpp"                // for CreatureClass, CreatureClass::LICH
#include "group.hpp"                 // for CreatureList, Group
#include "lasttime.hpp"              // for lasttime
#include "location.hpp"              // for Location
#include "mud.hpp"                   // for LT_SING, LT_HYPNOTIZE, osong
#include "mudObjects/container.hpp"  // for MonsterSet, PlayerSet, Container
#include "mudObjects/creatures.hpp"  // for Creature, CHECK_DIE
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, bonus, get_song_name, up
#include "random.hpp"                // for Random
#include "server.hpp"                // for Server, gServer
#include "socket.hpp"                // for Socket
#include "statistics.hpp"            // for Statistics
#include "stats.hpp"                 // for Stat
#include "structs.hpp"               // for osong_t, PFNCOMPARE


//*********************************************************************
//                      cmdSing
//*********************************************************************

int cmdSing(const std::shared_ptr<Creature>& creature, cmd* cmnd) {
    std::shared_ptr<Player> player = creature->getAsPlayer();
    long    i, t;
    int     (*fn) ();
    int     songno=0, c=0, match=0, n=0;

    if(player && !player->ableToDoCommand())
        return(0);

    if(!player || (player->getClass() !=  CreatureClass::BARD && !player->isCt())) {
        if(Random::get(0,10) || creature->isStaff()) {
            creature->print("You sing a song.\n");
            broadcast(creature->getSock(), creature->getRoomParent(), "%M sings a song.", creature.get());
        } else {
            creature->print("You sing off-key.\n");
            broadcast(creature->getSock(), creature->getRoomParent(), "%M sings off-key.", creature.get());
        }
        return(0);
    }

    player->clearFlag(P_AFK);

    if(player->getLevel() < 4 && !player->isCt()) {
        player->print("You have not practiced enough to do that yet.\n");
        return(0);
    }
    if(!player->canSpeak()) {
        player->printColor("^yYour lips move but no sound comes forth.\n");
        return(0);
    }

    i = player->lasttime[LT_SING].ltime + player->lasttime[LT_SING].interval;
    t = time(nullptr);
    if(i > t && !player->isCt()) {
        player->pleaseWait(i-t);
        return(0);
    }

    if(cmnd->num < 2) {
        songno = 0;
        match = 1;
    } else {
        do {
            if(!strcmp(cmnd->str[1], get_song_name(c))) {
                match = 1;
                songno = c;
                break;
            } else if(!strncmp(cmnd->str[1], get_song_name(c), strlen(cmnd->str[1]))) {
                match++;
                songno = c;
            }
            c++;
        } while(get_song_num(c) != -1);
    }
    if(match == 0) {
        player->print("That song does not exist.\n");
        return(0);
    } else if(match > 1) {
        player->print("Song name is not unique.\n");
        return(0);
    }
    if(!player->songIsKnown(songno)) {
        player->print("You don't know how the lyrics to that song!\n");
        return(0);
    }
    fn = get_song_function(songno);

    if((int(*)(const std::shared_ptr<Player>&, cmd*, char*, osong_t*))fn == songOffensive ||
        (int(*)(const std::shared_ptr<Player>&, cmd*, char*, osong_t*))fn == songMultiOffensive) {
        for(c = 0; osong[c].songno != get_song_num(songno); c++)
            if(osong[c].songno == -1)
                return(0);
        n = ((int(*)(std::shared_ptr<Creature>, cmd*, const char*, osong_t*))*fn) (player, cmnd, get_song_name(songno), &osong[c]);
    } else
        n = ((int(*)(std::shared_ptr<Creature>, cmd*))*fn) (player, cmnd);

    if(n) {
        player->unhide();
        player->lasttime[LT_SING].ltime = t;
        player->lasttime[LT_SING].interval = 120L;
    }

    return(0);
}

//*********************************************************************
//                      songMultiOffensive
//*********************************************************************

int songMultiOffensive(const std::shared_ptr<Player>& player, cmd* cmnd, char *songname, osong_t *oso) {
    int     len=0, ret=0;
    int     monsters=0, players=0;
    int     count=0;
    int     something_died=0;
    int     found_something=0;
    std::string lastname;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->songIsKnown(oso->songno)) {
        player->print("You don't know that song.\n");
        return(0);
    }
    auto room = player->getRoomParent();

    if(cmnd->num == 2) {
        monsters = 1;
    } else {
        len = strlen(cmnd->str[2]);
        if(!strncmp(cmnd->str[2], "monsters", len)) {
            monsters = 1;
        } else if(!strncmp(cmnd->str[2], "all", len)) {
            monsters = 1;
            players = 1;
        } else if(!strncmp(cmnd->str[2], "players", len)) {
            players = 1;
        } else {
            player->print("Usage: sing %s [<monsters>|<players>|<all>]\n", songname);
            return(0);
        }
    }
    cmnd->num = 3;
    if(monsters) {
        lastname = "";
        for(auto mIt = room->monsters.begin() ; mIt != room->monsters.end() ; ) {
            auto mons = (*mIt++);
            // skip caster's pet
            if(mons->isPet() && mons->getMaster() == player) {
                continue;
            }
            if(mons->getName() ==  lastname) {
                count++;
            } else {
                count = 1;
            }
            strncpy(cmnd->str[2], mons->getCName(), 25);
            cmnd->val[2] = count;
            lastname = mons->getName();
            ret = songOffensive(player, cmnd, songname, oso);
            if(ret == 0)
                return(found_something);
            // target died
            if(ret == 2) {
                count--;
                something_died = 1;
            }
            found_something = 1;
        }
    }
    if(players) {
        lastname = "";
        for(auto pIt = room->players.begin() ; pIt != room->players.end() ; ) {
            auto ply = pIt->lock();
            if(!ply) {
                pIt = room->players.erase(pIt);
                continue;
            }
            pIt++;
            // skip self
            if(!ply || ply == player) continue;
            if(ply->getName() == lastname) {
                count++;
            } else {
                count = 1;
            }
            strncpy(cmnd->str[2], ply->getCName(), 25);
            cmnd->val[2] = count;
            lastname = ply->getName();
            ret = songOffensive(player, cmnd, songname, oso);
            if(ret == 0)
                return(found_something);
            // target died
            if(ret == 2) {
                count--;
                something_died = 1;
            }
            found_something = 1;
        }
    }
    if(!found_something)
        player->print("Sing to whom?\n");

    return(found_something + something_died);
}

//*********************************************************************
//                      songOffensive
//*********************************************************************
// This function is called by all spells whose sole purpose is to do
// damage to a given creature.

int songOffensive(const std::shared_ptr<Player>& player, cmd* cmnd, char *songname, osong_t *oso) {
    std::shared_ptr<Player> pCreature=nullptr;
    std::shared_ptr<Creature> creature=nullptr;
    std::shared_ptr<BaseRoom> room = player->getRoomParent();
    int     m=0, bns=0;
    int dmg = 0;

    if(!player->ableToDoCommand())
        return(0);

    if(!player->songIsKnown(oso->songno)) {
        player->print("You don't know that song.\n");
        return(0);
    }

    if(!strcmp(songname, "destruction") && player->getLevel() < 13 && !player->isCt()) {
        player->print("You are not experienced enough to sing that.\n");
        return(0);
    }

    player->smashInvis();
    player->interruptDelayedActions();

    bns = (bonus(player->intelligence.getCur()) + bonus(player->piety.getCur())) / 2;

    // sing on self
    if(cmnd->num == 2) {
        player->print("You sing a song.\n");
        broadcast(player->getSock(), room, "%M sings a song.", player.get());

    // sing on monster or player
    } else {
        creature = room->findCreature(player, cmnd->str[2], cmnd->val[2], true, true);

        if(!creature || creature == player) {
            player->print("That's not here.\n");
            return(0);
        }

        pCreature = creature->getAsPlayer();

        if(!pCreature) {
            // Activates lag protection.
            if(player->flagIsSet(P_LAG_PROTECTION_SET))
                player->setFlag(P_LAG_PROTECTION_ACTIVE);
        }



        if(!player->canAttack(creature))
            return(0);

        if(pCreature) {
            if(player->vampireCharmed(pCreature) || (pCreature->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))) {
                player->print("You just can't bring yourself to do that.\n");
                return(0);
            }
        }


        if(songFail(player))
            return(0);


        // dmg = dice(oso->ndice, oso->sdice, oso->pdice + bns);
        dmg = (Random::get(7,14) + player->getLevel() * 2) + bns;
        dmg = std::max<int>(1, dmg);

        if(!pCreature) {
            // if(is_charm_crt(creature->name, player))
            // del_charm_crt(creature, player);
            m = std::min(creature->hp.getCur(), dmg);

            creature->getAsMonster()->adjustThreat(player, m);
        }



        player->lasttime[LT_SPELL].ltime = time(nullptr);
        player->lasttime[LT_SPELL].interval = 3L;
        player->updateAttackTimer(true, DEFAULT_WEAPON_DELAY);
        player->statistics.magicDamage(dmg, (std::string)"a song of " + songname);

        player->print("You sing a song of %s to %s.\n", songname, creature->getCName());
        player->printColor("Your song inflicted %s%d^x damage.\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg);
        broadcast(player->getSock(), creature->getSock(), room, "%M sings a song of %s to %N.", player.get(), songname, creature.get());
        creature->printColor("%M sings a song of %s to you.\n%M's song inflicted %s%d^x damage on you.\n",
            player.get(), songname, player.get(), creature->customColorize("*CC:DAMAGE*").c_str(), dmg);
        broadcastGroup(false, creature, "^M%M^x sings a song of %s to ^M%N^x for *CC:DAMAGE*%d^x damage, %s%s\n",
            player.get(), songname, creature.get(), dmg, creature->heShe(), creature->getStatusStr(dmg));

        if(player->doDamage(creature, dmg, CHECK_DIE))
            return(2);

    }
    return(1);
}

//*********************************************************************
//                      cmdSongs
//*********************************************************************

int cmdSongs(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target = player;
    int     test=0;

    if(player->isCt() && cmnd->num > 1) {
        cmnd->str[1][0] = up(cmnd->str[1][0]);
        target = gServer->findPlayer(cmnd->str[1]);
        test = 1;
        if(!target || !player->canSee(target)) {
            player->print("That player is not logged on.\n");
            return(0);
        }
    }

    songsKnown(player->getSock(), target, test);
    return(0);
}

//*********************************************************************
//                      songs_known
//*********************************************************************

int songsKnown(const std::shared_ptr<Socket> &sock, const std::shared_ptr<Player>& player, int test) {
    char            str[2048];
    char            sol[128][20];
    int             i=0, j=0;

    if(test)
        sprintf(str, "\n%s's Songs Known: ", player->getCName());
    else
        strcpy(str, "\nSongs known: ");

    for(i=0, j=0; i<gConfig->getMaxSong(); i++)
        if(player->songIsKnown(i))
            strcpy(sol[j++], get_song_name(i));
    if(!j)
        strcat(str, "None.");
    else {
        qsort((void *) sol, j, 20, (PFNCOMPARE) strcmp);
        for(i=0; i<j; i++) {
            strcat(str, "Song of ");
            strcat(str, sol[i]);
            strcat(str, ", ");
        }
        str[strlen(str) - 2] = '.';
        str[strlen(str) - 1] = 0;
    }
    sock->print("%s\n", str);
    return(0);
}

//*********************************************************************
//                      songFail
//*********************************************************************

int songFail(const std::shared_ptr<Player>& player) {
    int chance=0, n = Random::get(1, 100);
    player->computeLuck();

    chance = ((player->getLevel() + bonus(player->intelligence.getCur())) * 5) + 65;
    chance = chance * player->getLuck() / 50;
    if(n > chance) {
        player->print("You sing off key.\n");
        broadcast(player->getSock(), player->getParent(), "%M sings off key.", player.get());
        return(1);
    } else
        return(0);
}

//*********************************************************************
//                      songHeal
//*********************************************************************

int songHeal(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int      heal=0;

    player->print("You sing a song of healing.\n");
    player->print("Your song rejuvenates everyone in the room.\n");

    heal = Random::get<unsigned short>(player->getLevel(), player->getLevel()*2);

    if(player->getRoomParent()->magicBonus()) {
        player->print("The room's magical properties increase the power of your song.\n");
        heal += Random::get(5, 10);
    }

    for(const auto& pIt: player->getRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (ply->getClass() != CreatureClass::LICH) {
                if (ply != player)
                    ply->print("%M's song rejuvinates you.\n", player.get());
                player->doHeal(ply, heal);
            }
        }
    }

    for(const auto& mons : player->getRoomParent()->monsters) {
        if(mons->getClass() !=  CreatureClass::LICH && mons->isPet()) {
            mons->print("%M's song rejuvinates you.\n", player.get());
            player->doHeal(mons, heal);
        }
    }

    return(1);
}

//*********************************************************************
//                      songMPHeal
//*********************************************************************

int songMPHeal(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int      heal=0;

    player->print("You sing a song of magic restoration.\n");
    player->print("Your song mentally revitalizes everyone in the room.\n");

    heal = Random::get<unsigned short>(player->getLevel(), player->getLevel()*2)/2;

    if(player->getRoomParent()->magicBonus()) {
        player->print("The room's magical properties increase the power of your song.\n");
        heal += Random::get(5, 10);
    }
    for(const auto& pIt: player->getRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (ply->hasMp()) {
                if (ply != player)
                    ply->print("%M's song mentally revitalizes you.\n", player.get());
                ply->mp.increase(heal);
            }
        }
    }
    for(const auto& mons : player->getRoomParent()->monsters) {
        if(mons->hasMp() && mons->isPet()) {
            mons->print("%M's song mentally revitalizes you.\n", player.get());
            mons->mp.increase(heal);
        }
    }

    return(1);
}

//*********************************************************************
//                      songRestore
//*********************************************************************

int songRestore(const std::shared_ptr<Player>& player, cmd* cmnd) {
    int      heal=0;

    player->print("You sing a song of restoration.\n");
    player->print("Your song restores everyone in the room.\n");

    heal = Random::get<unsigned short>(player->getLevel(), player->getLevel()*2)*2;

    if(player->getRoomParent()->magicBonus()) {
        player->print("The room's magical properties increase the power of your song.\n");
        heal += Random::get(5, 10);
    }
    for(const auto& pIt: player->getRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (ply->getClass() != CreatureClass::LICH) {
                if (ply != player)
                    ply->print("%M's song restores your spirits.\n", player.get());
                player->doHeal(ply, heal);
                ply->mp.increase(heal / 2);
            }
        }
    }
    for(const auto& mons : player->getRoomParent()->monsters) {
        if(mons->getClass() !=  CreatureClass::LICH && mons->isPet()) {
            mons->print("%M's song restores your spirits.\n", player.get());
            player->doHeal(mons, heal);
            mons->mp.increase(heal/2);
        }
    }

    return(1);
}

//*********************************************************************
//                      songBless
//*********************************************************************

int songBless(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->print("You sing a song of holiness.\n");

    int duration = 600;
    if(player->getRoomParent()->magicBonus())
        duration += 300L;

    for(const auto& pIt: player->getRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (ply != player)
                ply->print("%M sings a song of holiness.\n", player.get());
            ply->addEffect("bless", duration, 1, player, true, player);
        }
    }
    for(const auto& mons : player->getRoomParent()->monsters) {
        if(mons->isPet()) {
            mons->print("%M sings a song of holiness.\n", player.get());
            mons->addEffect("bless", duration, 1, player, true, player);
        }
    }

    if(player->getRoomParent()->magicBonus())
        player->print("The room's magical properties increase the power of your song.\n");
    return(1);
}

//*********************************************************************
//                      songProtection
//*********************************************************************

int songProtection(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->print("You sing a song of protection.\n");
    int duration = std::max(300, 1200 + bonus(player->intelligence.getCur()) * 600);

    if(player->getRoomParent()->magicBonus())
        duration += 800L;

    for(const auto& pIt: player->getRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (ply != player)
                ply->print("%M sings a song of protection.\n", player.get());
            ply->addEffect("protection", duration, 1, player, true, player);
        }
    }
    for(const auto& mons : player->getRoomParent()->monsters) {
        if(mons->isPet()) {
            mons->print("%M sings a song of protection.\n", player.get());
            mons->addEffect("protection", duration, 1, player, true, player);
        }
    }

    if(player->getRoomParent()->magicBonus())
        player->print("The room's magical properties increase the power of your song.\n");
    return(1);
}

//*********************************************************************
//                      songFlight
//*********************************************************************

int songFlight(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;

    if(cmnd->num == 2) {
        player->print("Your song makes you feel light as a feather.\n");
        broadcast(player->getSock(), player->getParent(), "%M sings a song of flight.", player.get());

        target = player;

    } else {
        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findPlayer(player, cmnd, 2);
        if(!target) {
            player->print("You don't see that player here.\n");
            return(0);
        }
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M sings a song of flight to %N.\n", player.get(), target.get());
        target->print("%M sings a song of flight to you.\n",player.get());
        player->print("You sing %N a song of flight.\n", target.get());
    }

    if(player->getRoomParent()->magicBonus())
        player->print("The room's magical properties increase the power of your song.\n");

    target->addEffect("fly", -2, -2, player, true, player);
    return(1);
}

//*********************************************************************
//                      songRecall
//*********************************************************************

int songRecall(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> target=nullptr;
    std::shared_ptr<BaseRoom> newRoom=nullptr;


    if(player->getLevel() < 7) {
        player->print("You are not experienced enough to sing that song yet!\n");
        return(0);
    }
    // Sing on self
    if(cmnd->num == 2) {
        player->print("You sing a song of recall.\n");
        broadcast(player->getSock(), player->getParent(), "%M sings a song of recall.", player.get());

        if(!player->isStaff() && player->checkDimensionalAnchor()) {
            player->printColor("^yYour dimensional-anchor causes your song to go off-key!^w\n");
            return(1);
        }

        target = player;

    // Sing on another player
    } else {
        cmnd->str[2][0] = up(cmnd->str[2][0]);
        target = player->getParent()->findPlayer(player, cmnd, 2);
        if(!target) {
            player->print("That person is not here.\n");
            return(0);
        }

        player->print("You sing a song of recall on %N.\n", target.get());
        target->print("%M sings a song of recall on you.\n", player.get());
        broadcast(player->getSock(), target->getSock(), player->getParent(), "%M sings a song of recall on %N.", player.get(), target.get());

        if(!player->isStaff() && target->checkDimensionalAnchor()) {
            player->printColor("^y%M's dimensional-anchor causes your song to go off-key!^w\n", target.get());
            target->printColor("^yYour dimensional-anchor protects you from %N's song of recall!^w\n", player.get());
            return(1);
        }
    }

    newRoom = target->getRecallRoom().loadRoom(target);
    if(!newRoom) {
        player->print("Song failure.\n");
        return(0);
    }

    broadcast(target->getSock(), player->getRoomParent(), "%M disappears.", target.get());

    target->deleteFromRoom();
    target->addToRoom(newRoom);
    target->doPetFollow();
    return(1);
}

//*********************************************************************
//                      songSafety
//*********************************************************************

int songSafety(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Player> follower=nullptr;
    std::shared_ptr<BaseRoom> newRoom=nullptr;


    if(player->getLevel() < 10) {
        player->print("That song's just too hard for you to sing yet.\n");
        return(0);
    }
    player->print("You sing a song of safety.\n");
    broadcast(player->getSock(), player->getParent(), "%M sings a song of safety.", player.get());

    // handle everyone following singer
    Group* group = player->getGroup();
    if(group) {
        auto it = group->members.begin();
        while(it != group->members.end()) {
            if(auto crt = (*it).lock()) {
                it++;
                follower = crt->getAsPlayer();
                if(!follower) continue;
                if(!player->inSameRoom(follower)) continue;
                if(follower->isStaff()) continue;

                if(!player->isStaff() && follower->checkDimensionalAnchor()) {
                    player->printColor("^y%M's dimensional-anchor causes your song to go off-key!^w\n", follower.get());
                    follower->printColor("^yYour dimensional-anchor protects you from %N's song of safety!^w\n", player.get());
                    continue;
                }

                newRoom = follower->getRecallRoom().loadRoom(follower);
                if(!newRoom)
                    continue;
                broadcast(follower->getSock(), follower->getRoomParent(), "%M disappears.", follower.get());
                follower->deleteFromRoom();
                follower->addToRoom(newRoom);
                follower->doPetFollow();
            } else {
                it = group->members.erase(it);
            }

        }

    }

    if(!player->isStaff() && player->checkDimensionalAnchor()) {
        player->printColor("^yYour dimensional-anchor causes your song to go off-key!^w\n");
        return(1);
    }

    newRoom = follower->getRecallRoom().loadRoom(follower);
    if(newRoom) {
        broadcast(player->getSock(), player->getParent(), "%M disappears.", player.get());
        player->courageous();
        player->deleteFromRoom();
        player->addToRoom(newRoom);
        player->doPetFollow();
    }
    return(1);
}

//*********************************************************************
//                      cmdCharm
//*********************************************************************

int cmdCharm(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Creature> creature=nullptr;
    int     dur, chance;
    long    i, t;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(!player->knowsSkill("charm")) {
        player->print("Sorry, but you aren't very charming.\n");
        return(0);
    }

    if(!player->canSpeak()) {
        player->printColor("^yYour lips move but no sound comes forth.\n");
        return(0);
    }

    i = LT(player, LT_HYPNOTIZE);
    t = time(nullptr);
    if(i > t && !player->isDm()) {
        player->pleaseWait(i-t);
        return(0);
    }

    int level = (int)player->getSkillLevel("charm");
    dur = 300 + Random::get(1, 30) * 10 + bonus(player->piety.getCur()) * 30 + level * 5;

    //cmnd->str[1][0] = up(cmnd->str[1][0]);

    if(!(creature = player->findVictim(cmnd, 1, true, false, "Charm whom?\n", "You don't see that here.\n")))
        return(0);

    if(!player->canAttack(creature))
        return(0);

    if( creature->isPlayer() &&
        (   player->vampireCharmed(creature->getAsPlayer()) ||
            (creature->hasCharm(player->getName()) && player->flagIsSet(P_CHARMED))
        ))
    {
        player->print("But they are already your good friend!\n");
        return(0);
    }

    if(creature->isMonster() && (creature->flagIsSet(M_NO_CHARM) || (creature->intelligence.getCur() <30))) {
        player->print("Your charm fails.\n");
        return(0);
    }


    if( creature->isUndead() &&
        !player->checkStaff("Your charm will not work on undead beings.\n") )
        return(0);


    if(creature->isMonster() && creature->getAsMonster()->isEnemy(player)) {
        player->print("Not while you are already fighting %s.\n", creature->himHer());
        return(0);
    }

    player->smashInvis();
    player->lasttime[LT_HYPNOTIZE].ltime = t;
    player->lasttime[LT_HYPNOTIZE].interval = 600L;


    chance = std::min(90, 40 + ((level) - (creature->getLevel())) * 20 + 4 * bonus(player->intelligence.getCur()));

    if(creature->flagIsSet(M_PERMANENT_MONSTER) && creature->isMonster())
        chance /= 2;

    if(player->isDm())
        chance = 101;
    if((creature->isUndead() || chance < Random::get(1, 100)) && chance != 101) {
        player->print("Your song has no effect on %N.\n", creature.get());
        player->checkImprove("charm", false);
        broadcast(player->getSock(), player->getParent(), "%M sings off key.",player.get());
        if(creature->isMonster()) {
            creature->getAsMonster()->addEnemy(player);
            return(0);
        }
        creature->printColor("^m%M tried to charm you.\n", player.get());
        return(0);
    }

    if(!creature->isCt()) {
        if(creature->chkSave(MEN, player, 0)) {
            player->printColor("^yYour charm failed!\n");
            creature->print("Your mind tingles as you brush off %N's charm.\n", player.get());
            player->checkImprove("charm", false);
            return(0);
        }
    }

    if( (creature->isPlayer() && creature->isEffected("resist-magic")) ||
        (creature->isMonster() && creature->isEffected("resist-magic"))
    )
        dur /= 2;

    player->print("Your song charms %N.\n", creature.get());
    player->checkImprove("charm", true);
    broadcast(player->getSock(), creature->getSock(), player->getRoomParent(), "%M sings to %N!", player.get(), creature.get());
    creature->print("%M's song charms you.\n", player.get());
    player->addCharm(creature);

    creature->lasttime[LT_CHARMED].ltime = time(nullptr);
    creature->lasttime[LT_CHARMED].interval = dur;

    creature->setFlag(creature->isPlayer() ? P_CHARMED : M_CHARMED);
    return(0);
}

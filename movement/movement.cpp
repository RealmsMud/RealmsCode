/*
 * movement.cpp
 *   Functions dealing with player movement.
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

#include <cctype>                      // for isdigit
#include <cstdlib>                     // for atoi
#include <cstring>                     // for strncmp, strstr, strcpy
#include <ctime>                       // for time
#include <list>                        // for list, operator==, _List_iterator
#include <map>                         // for operator==, _Rb_tree_const_ite...
#include <set>                         // for set
#include <string>                      // for string, allocator, operator==
#include <string_view>                 // for operator==, basic_string_view
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/token_functions.hpp>
#include <boost/tokenizer.hpp>

#include "area.hpp"                    // for MapMarker, Area, AreaTrack
#include "catRef.hpp"                  // for CatRef
#include "catRefInfo.hpp"              // for CatRefInfo
#include "cmd.hpp"                     // for cmd
#include "commands.hpp"                // for cmdUseWand, tollcost, cmdClose
#include "config.hpp"                  // for Config, gConfig
#include "creatureStreams.hpp"         // for Streamable, operator<<, ColorOff
#include "factions.hpp"                // for Faction
#include "flags.hpp"                   // for X_LOCKED, P_AFK, P_SITTING
#include "global.hpp"                  // for CreatureClass, CreatureClass::...
#include "group.hpp"                   // for Group, GROUP_LEADER, CreatureList
#include "hooks.hpp"                   // for Hooks
#include "lasttime.hpp"                // for lasttime
#include "location.hpp"                // for Location
#include "magic.hpp"                   // for S_KNOCK
#include "move.hpp"                    // for deletePortal, tooFarAway, useP...
#include "mud.hpp"                     // for LT_MOVED, LT_PLAYER_STUNNED
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/container.hpp"    // for MonsterSet, Container, PlayerSet
#include "mudObjects/creatures.hpp"    // for Creature, PetList, DEL_ROOM_DE...
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/monsters.hpp"     // for Monster
#include "mudObjects/objects.hpp"      // for Object, ObjectType, ObjectType...
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "proto.hpp"                   // for broadcast, findExit, isCt, isDm
#include "random.hpp"                  // for Random
#include "server.hpp"                  // for Server, gServer
#include "size.hpp"                    // for NO_SIZE
#include "stats.hpp"                   // for Stat
#include "structs.hpp"                 // for ALCOHOL_DRUNK, ALCOHOL_INEBRIATED
#include "track.hpp"                   // for Track
#include "unique.hpp"                  // for Unique
#include "xml.hpp"                     // for loadRoom
#include "toNum.hpp"

class EffectInfo;

//*********************************************************************
//                      tooFarAway
//*********************************************************************

bool Move::tooFarAway(const std::shared_ptr<BaseRoom>& pRoom, const std::shared_ptr<BaseRoom>& tRoom, bool track) {
    if(pRoom == tRoom)
        return(false);
    if(!pRoom || !tRoom || pRoom->flagIsSet(R_JAIL) || tRoom->flagIsSet(R_JAIL))
        return(true);

    const CatRefInfo*   pr = gConfig->getCatRefInfo(pRoom);
    if(!pr)
        return(true);
    const CatRefInfo*   tr = gConfig->getCatRefInfo(tRoom);
    if(!tr)
        return(true);

    if(pr->getTeleportZone() != tr->getTeleportZone())
        return(true);
    if(track && pr->getTrackZone() != tr->getTrackZone())
        return(true);

    return(false);
}

bool Move::tooFarAway(const std::shared_ptr<Creature>& player, const std::shared_ptr<BaseRoom>& room) {
    if(player->isStaff())
        return(false);

    if(!room || Move::tooFarAway(player->getRoomParent(), room, false)) {
        player->print("That location is too far away.\n");
        return(true);
    }

    return(false);
}

bool Move::tooFarAway(const std::shared_ptr<Creature>& player, const std::shared_ptr<Creature>& target, const std::string& action) {
    if(player->isStaff())
        return(false);

    std::shared_ptr<BaseRoom> tRoom = target->getRoomParent();
    // target is offline, so we need to load their room
    if(!tRoom) {
        std::shared_ptr<UniqueRoom> room=nullptr;        if(target->currentLocation.room.id && loadRoom(target->currentLocation.room, room))
            tRoom = room;
    }

    if(!tRoom || Move::tooFarAway(player->getRoomParent(), tRoom, action == "track")) {
        player->print("%M is too far away to %s.\n", target.get(), action.c_str());
        return(true);
    }

    return(false);
}

//*********************************************************************
//                      broadcast
//*********************************************************************

void Move::broadcast(const std::shared_ptr<Creature>& player, const std::shared_ptr<const Container>& container, bool ordinal, const std::string& exit, bool hiddenExit) {
    std::string strAction = Move::getString(player, ordinal, exit.c_str());
    bool noShow = (player->pFlagIsSet(P_DM_INVIS));

    if(!noShow) {
        if(player->isMonster() || (player->isPlayer() && !player->isEffected("mist"))) {
            if(hiddenExit)
                ::broadcast(player->getSock(), container, "%M slips out of sight.", player.get());
            else
                ::broadcast(player->getSock(), container, "%M %s %s^x.", player.get(), strAction.c_str(), exit.c_str());
        } else if(player->isEffected("mist") && !player->flagIsSet(P_DM_INVIS)) {
            if(hiddenExit)
                ::broadcast(player->getSock(), container, "A light mist slips out of sight.");
            else
                ::broadcast(player->getSock(), container, "A light mist %s %s^x.", strAction.c_str(), exit.c_str());
        }
    }

    if(noShow || hiddenExit) {
        if(player->isDm())
            ::broadcast(isDm, player->getSock(), container, "*STAFF* %M %s %s^x.", player.get(), strAction.c_str(), exit.c_str());
        if(player->getClass() == CreatureClass::CARETAKER)
            ::broadcast(isCt, player->getSock(), container, "*STAFF* %M %s %s^x.", player.get(), strAction.c_str(), exit.c_str());
        if(!player->isCt())
            ::broadcast(isStaff, player->getSock(), container, "*STAFF* %M %s %s^x.", player.get(), strAction.c_str(), exit.c_str());
    }
}


//*********************************************************************
//                      formatFindExit
//*********************************************************************
// This function allows for multi-word desc-only exits.

std::string Move::formatFindExit(cmd* cmnd) {
    std::string str;
    int     i=0;

    str = "";
    for(i=1; i<cmnd->num; i++) {
        if(cmnd->str[i][0]) {
            str += cmnd->str[i];
            str += " ";
        }
    }
    if(str.length()>1)
        str = str.substr(0, str.length() - 1);
    return(str);
}


//*********************************************************************
//                      isSneaking
//*********************************************************************
// if we're sneaking

bool Move::isSneaking(cmd* cmnd) {
    return(!strncmp(cmnd->str[0], "sn", 2));
}

//*********************************************************************
//                      isOrdinal
//*********************************************************************
// if it's an ordinal move

bool Move::isOrdinal(cmd* cmnd) {
    // Ordinal if not go, enter, exit, or sneak
    return( strncmp(cmnd->str[0], "go", 2) != 0 &&
            strncmp(cmnd->str[0], "en", 2) &&
            strncmp(cmnd->str[0], "ex", 2) != 0 &&
            !Move::isSneaking(cmnd) );
}

//*********************************************************************
//                      recycle
//*********************************************************************
// given the room we just left (everyone is out) this code handles the
// recycling of the room

std::shared_ptr<AreaRoom> Move::recycle(std::shared_ptr<AreaRoom> room, const std::shared_ptr<Exit>& exit) {
    if(auto area = room->area.lock()) {
        if(room->canDelete()) {
            area->moveMapMarker(room, exit->target.mapmarker);
        } else {
            room = std::make_shared<AreaRoom>(area);
            area->setMapMarker(room, exit->target.mapmarker);
        }

    }
    return(room);
}

//*********************************************************************
//                      update
//*********************************************************************
// updates some settings on the character once they move

void Move::update(const std::shared_ptr<Player>& player) {
    if( player->getRoomParent()->isUnderwater() && !player->isEffected("free-action")) {
        player->lasttime[LT_MOVED].ltime = time(nullptr);
        player->lasttime[LT_MOVED].interval = 2L;
    }

    if(player->flagIsSet(P_SITTING))
        player->stand();

    player->clearFlag(P_JUST_TRAINED);

    player->lasttime[LT_MOVED].misc++;
}


//*********************************************************************
//                      broadMove
//*********************************************************************
// announces to the room we're leaving that we're leaving

void Move::broadMove(const std::shared_ptr<Creature>& player, const std::shared_ptr<Exit>& exit, cmd* cmnd, bool sneaking) {

    if(!exit->getEnter().empty())
        player->printColor("%s\n", exit->getEnter().c_str());

    // sneak failure is decided earlier
    if(!sneaking) {
        Move::broadcast(
            player,
            player->getRoomParent(),
            Move::isOrdinal(cmnd),
            exit->getName(),
            exit->flagIsSet(X_SECRET) || exit->isConcealed() || exit->flagIsSet(X_DESCRIPTION_ONLY)
        );
    } else {
        if(player->isDm())
            ::broadcast(isDm, player->getSock(), player->getRoomParent(), "*DM* %M snuck to the %s^x.", player.get(), exit->getCName());
        if(player->getClass() == CreatureClass::CARETAKER)
            ::broadcast(isCt, player->getSock(), player->getRoomParent(), "*DM* %M snuck to the %s^x.", player.get(), exit->getCName());
        if(!player->isCt())
            ::broadcast(isStaff, player->getSock(), player->getRoomParent(), "*DM* %M snuck to the %s^x.", player.get(), exit->getCName());
        player->checkImprove("sneak", true);
    }
}



//*********************************************************************
//                      track
//*********************************************************************
// takes care of any tracks we leave

bool Move::track(const std::shared_ptr<UniqueRoom>& room, const MapMarker& mapmarker, const std::shared_ptr<Exit>& exit, const std::shared_ptr<Player>& player, bool reset) {
    if(room && room->flagIsSet(R_PERMENANT_TRACKS))
        return(reset);
    if(exit->flagIsSet(X_DESCRIPTION_ONLY) || exit->flagIsSet(X_PORTAL))
        return(reset);

    if( player &&
        !player->isEffected("fly") &&
        !player->isEffected("levitate") &&
        !player->isEffected("pass-without-trace") &&
        !player->isStaff())
    {
        Track *track=nullptr;

        if(room)
            track = &room->track;
        else if(mapmarker.isValid()){

            std::shared_ptr<Area> area = gServer->getArea(mapmarker.getArea());
            if(area) {
                track = area->getTrack(mapmarker);
                // only make tracks if they will last longer than 0 minutes
                if(!track && area->getTrackDuration(mapmarker)) {
                    auto aTrack = std::make_shared<AreaTrack>();

                    aTrack->mapmarker = mapmarker;
                    aTrack->setDuration(area->getTrackDuration(mapmarker));

                    area->addTrack(aTrack);
                    track = &aTrack->track;
                }
            }
        }

        if(track) {
            if(!reset) {
                track->setNum(0);
                track->setSize(NO_SIZE);
            }
            track->setDirection(exit->getName());
            if(player->getSize() > track->getSize())
                track->setSize(player->getSize());
            track->setNum(track->getNum()-1);
            return(true);
        }
    }
    return(reset);
}

void Move::track(const std::shared_ptr<UniqueRoom>& room, const MapMarker& mapmarker, const std::shared_ptr<Exit>& exit, const std::shared_ptr<Player>&leader, std::list<std::shared_ptr<Creature>> *followers) {
    std::list<std::shared_ptr<Creature>>::iterator it;
    bool    reset=false;

    if(!room && !mapmarker.isValid())
        return;
    if(room && room->flagIsSet(R_PERMENANT_TRACKS))
        return;
    if(exit->flagIsSet(X_DESCRIPTION_ONLY))
        return;

    // the leader is not stored in the list
    reset = Move::track(room, mapmarker, exit, leader, reset);

    for(it = followers->begin() ; it != followers->end() ; it++)
        reset = Move::track(room, mapmarker, exit, (*it)->getAsPlayer(), reset);
}


//*********************************************************************
//                      sneak
//*********************************************************************
// handles our attempted sneaking

bool Move::sneak(const std::shared_ptr<Player>& player, bool sneaking) {

    if(sneaking && player->isEffected("mist"))
        player->setFlag(P_SNEAK_WHILE_MISTED);

    if(!player->isStaff() && !player->isEffected("mist")) {

        // see if they failed sneaking or not
        if(sneaking && (
                !player->flagIsSet(P_HIDDEN) ||
                Random::get(1, 100) > player->getSneakChance()
            )
        ) {
            player->checkImprove("sneak", false);
            player->print("You failed to sneak.\n");
            sneaking = false;
        }

    }

    if(!sneaking)
        player->unhide();

    return(sneaking);
}

//*********************************************************************
//                      canEnter
//*********************************************************************
// this function determines if we're allowed to go in the exit provided
// it should print the reason for failure

bool Move::canEnter(const std::shared_ptr<Player>& player, const std::shared_ptr<Exit>& exit, bool leader) {
    // this also keeps builders out of overland rooms
    if(!player->checkBuilder(exit->target.room))
        return(false);

    // sending true to this function will print the reason
    // why they can't enter
    if(!player->canEnter(exit, true))
        return(false);
    for(const auto& pet : player->pets) {
        if(pet && !pet->canEnter(exit) && !player->checkStaff("%M cannot go that way.\n", pet.get()))
            return(false);
    }

    if(player->isStaff())
        return(true);

    // This is handled here and not in canEnter. If it were in canEnter,
    // they wouldn't be able to flee past the monster.
    for(const auto& mons : player->getRoomParent()->monsters) {
        if( mons->flagIsSet(M_BLOCK_EXIT) && mons->isEnemy(player) && mons->canSee(player)) {
            player->print("%M blocks your exit.\n", mons.get());
            return(false);
        }
    }

    if( (exit->flagIsSet(X_NEEDS_CLIMBING_GEAR) || exit->flagIsSet(X_CLIMBING_GEAR_TO_REPEL)) &&
        !player->isEffected("levitate") &&
        !player->isEffected("mist"))
    {
        int fall = (exit->flagIsSet(X_DIFFICULT_CLIMB) ? 50 : 0) + 50 - player->getFallBonus();

        if(Random::get(1, 100) < fall) {
            int dmg = Random::get(5, 15 + fall / 10);

            player->printColor("You fell and hurt yourself for %s%d^x damage.\n", player->customColorize("*CC:DAMAGE*").c_str(), dmg);
            ::broadcast(player->getSock(), player->getParent(), "%M fell down.", player.get());
            broadcastGroup(false, player, "%M fell and took *CC:DAMAGE*%d^x damage, %s%s\n", player.get(), dmg, player->heShe(), player->getStatusStr(dmg));

            player->hp.decrease(dmg);
            if(player->hp.getCur() <= 0) {
                player->print("You fell to your death.\n");
                player->hp.setCur(0);
                ::broadcast(player->getSock(), player->getParent(), "%M died from the fall.\n", player.get());
                player->die(FALL);
                return(false);
            }

            if(exit->flagIsSet(X_NEEDS_CLIMBING_GEAR)) {
                player->print("You need climbing gear to go that way.\n");
                return(false);
            }
        }
    }

    // don't follow someone to an incorrect bound room
    // only if you aren't trying to walk yourself through
    if( !leader &&
        exit->flagIsSet(X_TO_BOUND_ROOM) &&
        player->getGroup() &&
        player->getGroup()->getLeader() &&
        player->getGroup()->getLeader()->getAsPlayer() &&
        player->bound != player->getGroup()->getLeader()->getAsPlayer()->bound
    )
        return(false);

    return(true);
}


//*********************************************************************
//                      canMove
//*********************************************************************
// this runs the checks to see if we're even able to go anywhere
// failure should print a message

bool Move::canMove(const std::shared_ptr<Player>& player, cmd* cmnd) {

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(false);

    if(player->isStaff())
        return(true);

    if(player->isMagicallyHeld(true)) {
        return(false);
    }

    if( !player->getRoomParent()->flagIsSet(R_LIMBO) && (
            player->getTotalBulk() > player->getMaxBulk() ||
            player->getWeight() > player->maxWeight()
        )
    ) {
        player->print("You are carrying too many things to move!\n");
        return(false);
    }

    if(!player->flagIsSet(P_STUNNED))
        if(player->checkConfusion())
            return(false);

    if(Move::isSneaking(cmnd)) {
        if(!player->knowsSkill("sneak")) {
            player->print("You don't know how to sneak effectively.\n");
            return(false);
        }
        if(!player->flagIsSet(P_HIDDEN) && !player->isEffected("mist")) {
            player->print("You need to hide first.\n");
            return(false);
        }
        if(player->getRoomParent()->isUnderwater()) {
            player->print("You can't sneak here! You're too busy swimming!\n");
            return(false);
        }
        if(player->inCombat()) {
            player->print("How do you expect to sneak in the middle of battle?!\n");
            return(false);
        }
        if(player->flagIsSet(P_SITTING)) {
            player->print("You have to stand up first to properly sneak.\n");
            return(false);
        }
    }


    int     chance=0, moves=0;
    long    t = time(nullptr);
    long    s = LT(player, LT_SPELL);
    long    u = LT(player, LT_MOVED);


    if(u - t > 0 && player->getRoomParent()->isUnderwater() && !player->isEffected("free-action")) {
        player->pleaseWait(u - t);
        return(false);
    }

    if(Move::isSneaking(cmnd)) {
        if(t < s) {
            player->pleaseWait(s - t);
            return(false);
        }
        if(!player->checkAttackTimer())
            return(false);
    } else {
        chance = std::max<int>(1, ((5+(player->dexterity.getCur()/10)*3) - player->getArmorWeight() + player->strength.getCur()));

        if(player->getClass() == CreatureClass::RANGER || player->getClass() == CreatureClass::DRUID)
            chance += 5;
        if(player->isEffected("levitate"))
            chance += 5;

        if( player->getRoomParent()->flagIsSet(R_DIFFICULT_TO_MOVE) &&
            !player->isEffected("fly") &&
            !player->isEffected("mist") &&
            !player->isEffected("free-action")
        )
            moves = 1;
        else
            moves = 3;

        // check for speed move

        // TODO: Tweak this check
        if( player->lasttime[LT_MOVED].ltime == t || (
                player->getRoomParent()->flagIsSet(R_DIFFICULT_TO_MOVE) &&
                !player->isEffected("fly") &&
                !player->isEffected("free-action") &&
                !player->isEffected("mist") &&
                Random::get(1,100) > chance
            ))
        {
            if(player->lasttime[LT_MOVED].misc > moves) {
                bool wait = false;
                if(moves == 1) {
                    if(player->getRoomParent()->flagIsSet(R_EARTH_BONUS)) {
                        player->print("You are stuck in the mud!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got stuck in the mud!", player.get());
                    } else if(player->getRoomParent()->flagIsSet(R_AIR_BONUS)) {
                        player->print("The strong wind knocks you from your feet!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got blown over by the wind!", player.get());
                    } else if(player->getRoomParent()->flagIsSet(R_FIRE_BONUS)) {
                        player->print("You are knocked down by heat waves!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got knocked down by heat waves!", player.get());
                    } else if(player->getRoomParent()->flagIsSet(R_WATER_BONUS)) {
                        player->print("Strong water currents make you lose balance!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got knocked down by the current!", player.get());
                    } else if(player->getRoomParent()->flagIsSet(R_COLD_BONUS) || player->getRoomParent()->isWinter()) {
                        player->print("You are stuck in the ice and snow!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got stuck in the ice and snow!", player.get());
                    } else if(player->getRoomParent()->flagIsSet(R_ELEC_BONUS)) {
                        player->print("Strong magnetic forces knock you down!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got knocked down by magnetic forces!", player.get());
                    } else {
                        player->print("You are stuck!!\n");
                        ::broadcast(player->getSock(), player->getParent(), "%M got stuck!", player.get());
                    }
                    wait = true;
                } else {
                    // Speedwalking is fine as long as there are no aggros in the room; if there are
                    // there's a chance to trip them up
                    for(const auto& monster : player->getParent()->monsters) {
                        if(monster->willAggro(player) || monster->isEnemy(player)) {
                            *player << ColorOn << setf(CAP) << monster << " startles you.\n" << ColorOff;
                            wait = true;
                            break;
                        }
                    }
                }

                if(wait) {
                    player->pleaseWait(1);
                    return(false);
                }
            }
        } else {
            player->lasttime[LT_MOVED].ltime = t;
            player->lasttime[LT_MOVED].misc = 1;
        }


        if(player->lasttime[LT_PLAYER_STUNNED].ltime+(player->lasttime[LT_PLAYER_STUNNED].interval) > t) {
            player->pleaseWait((player->lasttime[LT_PLAYER_STUNNED].interval + 1) - t + player->lasttime[LT_PLAYER_STUNNED].ltime - 1);
            return(false);
        }

        // fix walking bug here
        if(player->getLTAttack()+3 > t) {
            player->pleaseWait(3-t+player->getLTAttack());
            return(false);
        // and here
        } else if(player->lasttime[LT_SPELL].ltime+3 > t) {
            player->pleaseWait(3-t+player->lasttime[LT_SPELL].ltime);
            return(false);
        // this fixes walking steal
        } else if(player->lasttime[LT_STEAL].ltime+3 > t) {
            player->pleaseWait(3-t+player->lasttime[LT_STEAL].ltime);
            return(false);
        }

    }

    return(true);
}

//*********************************************************************
//                      getExit
//*********************************************************************
// this looks through the exits in the room and finds the one we want
// to go to

std::shared_ptr<Exit> Move::getExit(std::shared_ptr<Creature> player, cmd* cmnd) {
    std::shared_ptr<BaseRoom> room = player->getRoomParent();
    std::shared_ptr<Exit> exit=nullptr;

    if(!room)
        return(nullptr);

    if(player->isPet())
        player = player->getMaster();

    if(Move::isOrdinal(cmnd)) {
        for(const auto& ext : room->exits) {
            if( ext->getName() == cmnd->str[1] && player->canSee(ext) && !(!player->isStaff() && ext->flagIsSet(X_DESCRIPTION_ONLY))) {
                exit = ext;
                break;
            }
        }
    } else {
        if(cmnd->fullstr.length() < 3)
            return(nullptr);

        exit = findExit(player, Move::formatFindExit(cmnd), cmnd->val[cmnd->num-1], room);
    }
    return(exit);
}

//*********************************************************************
//                      drunkenStumble
//*********************************************************************
// whether or not a person should stumble when drunk

bool drunkenStumble(const EffectInfo* effect) {
    AlcoholState state = getAlcoholState(effect);
    switch(state) {
    case ALCOHOL_TIPSY:
        // 33%
        return(!Random::get(0,2));
    case ALCOHOL_DRUNK:
        // 75%
        return Random::get(0, 3) != 0;
    case ALCOHOL_INEBRIATED:
        // 100%
        return(true);
    default:
        // 0%
        return(false);
    }
}

//*********************************************************************
//                      getString
//*********************************************************************
// gives us the text of the movement string

std::string Move::getString(const std::shared_ptr<Creature>& creature, bool ordinal, std::string_view exit) {
    std::string str;
    int     num=0;

    if(creature->getAsPlayer()) {

        if(creature->isStaff())
            str = "wandered to the";
        else if(creature->getRoomParent()->isUnderwater())
            str = "swam to the";
        else if(creature->isEffected("fly"))
            str = "flew to the";
        else if(creature->isEffected("levitate") || creature->isEffected("mist"))
            str = "floated to the";
        else if(creature->isEffected("confusion") || drunkenStumble(creature->getEffect("drunkenness")))
            str = "stumbled to the";
        // Not ordinal, not up, not down, and not out
        else if( !ordinal &&
            exit != "up" &&
            exit != "down" &&
            exit != "out"
        )
            str = "went to the";
        else
            str = "left";

    } else {

        if( creature->movetype[0][0] ||
            creature->movetype[1][0] ||
            creature->movetype[2][0]
        ) {

            do {
                num = Random::get(1, 3);
                if(creature->movetype[num-1][0])
                    str = creature->movetype[num-1];
            } while(!creature->movetype[num-1][0]);

        } else if(creature->getRoomParent()->isUnderwater())
            str = "swam to the";
        else if(creature->isEffected("fly"))
            str = "flew to the";
        else if(creature->isEffected("levitate"))
            str = "floats to the";
        else
            str = "wandered";

    }

    return(str);
}



//*********************************************************************
//                      checkFollowed
//*********************************************************************
// this sees if any monsters plan on following the people leaving the
// room, adding them to the list if they want to tag along for the ride

void Move::checkFollowed(const std::shared_ptr<Player>& player, const std::shared_ptr<Exit>& exit, const std::shared_ptr<BaseRoom>& room, std::list<std::shared_ptr<Creature>> *followers) {
    std::shared_ptr<Monster> target=nullptr;
    if(!room)
        return;

    auto mIt = room->monsters.begin();
    while(mIt != room->monsters.end()) {
        target = (*mIt++);

        if(!target->flagIsSet(M_FOLLOW_ATTACKER) || target->flagIsSet(M_DM_FOLLOW))
            continue;
        if(!target->hasEnemy())
            continue;

        std::shared_ptr<Creature> crt = target->getTarget(false);
        if(!crt || crt != player) continue;

        // Protect the newbie areas from aggro-dragging
        if(target->flagIsSet(M_AGGRESSIVE) && exit->flagIsSet(X_PASSIVE_GUARD))
            continue;

        if(!target->canSee(player))
            continue;
        if(!target->canEnter(exit))
            continue;

        if(Random::get(1,20) > 10 - (player->dexterity.getCur()/10) + target->dexterity.getCur()/10)
            continue;

        player->print("%M followed you.\n", target.get());
        ::broadcast(player->getSock(), room, "%M follows %N.", target.get(), player.get());

        // prevent players from continuously creating new monsters
        if(target->flagIsSet(M_PERMANENT_MONSTER))
            target->diePermCrt();

        target->clearFlag(M_PERMANENT_MONSTER);
        gServer->addActive(target);

        target->deleteFromRoom();

        followers->push_back(target);
    }

}



//*********************************************************************
//                      finish
//*********************************************************************
// this finishes up all the work of adding people to the room

void Move::finish(const std::shared_ptr<Creature>& creature, const std::shared_ptr<BaseRoom>& room, bool self, std::list<std::shared_ptr<Creature>> *followers) {
    std::shared_ptr<Player> player = creature->getAsPlayer();
    std::shared_ptr<Monster> monster = creature->getAsMonster();

    if(player)
        player->addToRoom(room);
    else
        monster->addToRoom(room);

    if(followers && !followers->empty()) {
        while(!followers->empty()) {
            Move::finish(followers->front(), room, false, nullptr);
            followers->pop_front();
        }
        followers->clear();
    }

    if(player && room->isUniqueRoom())
        player->checkTraps(room->getAsUniqueRoom(), self);
}


//*********************************************************************
//                      getRoom
//*********************************************************************
// loads up the room. handles moving into the room through an exit
// (walk/flee), looking into the room (scout/yell), and appearing in
// the room (teleport)
// creature is allowed to be null if we're just trying to load the room
// out of the exit.

bool Move::getRoom(const std::shared_ptr<Creature>& creature, const std::shared_ptr<Exit>& exit, std::shared_ptr<BaseRoom>& newRoom, bool justLooking, MapMarker* teleport, bool recycle) {

    std::shared_ptr<Player> player = nullptr;
    if(creature)
       player = creature->getAsPlayer();
    Location l;

    // pets can go where players can go
    if(!player && creature && creature->isPet())
        player = creature->getPlayerMaster();

    // find out where the exit actually points

    if(creature && exit && (
        exit->flagIsSet(X_TO_BOUND_ROOM) ||
        exit->flagIsSet(X_TO_PREVIOUS) ||
        exit->flagIsSet(X_TO_STORAGE_ROOM)
    ) )
    {
        // flags can cause us to completely ignore where the exit points
        if(exit->flagIsSet(X_TO_PREVIOUS))
            l = creature->previousRoom;
        else if(player && exit->flagIsSet(X_TO_STORAGE_ROOM)) {
            l.room = gConfig->getSingleProperty(player, PROP_STORAGE);
            if(!l.room.id) {
                player->print("You must first purchase a storage room to go to this exit.\n");
                return(false);
            } else if(l.room.id == -1) {
                player->print("You may not enter that room when you are affiliated with more than one\n");
                player->print("storage room. Type \"property\" for instructions on how to remove yourself\n");
                player->print("as partial owner from other storage rooms if you no longer wish to be\n");
                player->print("affiliated with them.\n");
                return(false);
            }
        } else if(player && exit->flagIsSet(X_TO_BOUND_ROOM))
            l = player->getBound();
        else
            l.room = creature->currentLocation.room;
    } else if(!exit || teleport) {
        // if we're teleporting
        l.mapmarker = *teleport;
    } else if(exit->target.mapmarker.getArea()) {
        l.mapmarker = exit->target.mapmarker;
    } else {
        l.room = exit->target.room;
    }

    if(l.mapmarker.getArea()) {
        std::shared_ptr<Area> area = gServer->getArea(l.mapmarker.getArea());
        if(!area) {
            if(!teleport && creature) {
				creature->print("Off the map in that direction.\n");
                if(creature->isStaff())
                    creature->printColor("^eMove::getRoom failed on an area room.\n");
            }
            return(false);
        }
        // if we're in a unique room, don't activate any unique-room triggers on
        // the overland unless we're teleporting
        if(teleport || !creature || !creature->inUniqueRoom())
            l.room = area->getUnique(l.mapmarker);
        if(!l.room.id) {
            // don't recycle if we are teleporting or leaving
            // a unique room
            if( teleport ||
                (creature && creature->inAreaRoom() && creature->getAreaRoomParent()->unique.id)
            )
                recycle = false;
            // if we're only looking, don't use the function that will create a new room
            // if one doesnt already exit
            if(justLooking)
                (newRoom) = area->getRoom(l.mapmarker);
            else
                (newRoom) = area->loadRoom(creature, l.mapmarker, recycle);
            if(!newRoom)
                return(false);
            // getUnique gets called again later, so skip the decrement process here
            // or the compass will use 2 shots
            if(teleport || !creature || !creature->inUniqueRoom())
                l.room = (newRoom)->getAsAreaRoom()->getUnique(creature, true);
        }
    }

    if(l.room.id) {
        std::shared_ptr<UniqueRoom> uRoom = nullptr;
        if( (creature && creature->inUniqueRoom() && l.room == creature->currentLocation.room) ||
            !loadRoom(l.room, uRoom) )
        {
            if(!teleport && creature)
                creature->print("Off map in that direction.\n");
            return(false);
        }
        newRoom = uRoom;
        if(creature) {
            // sending true to this function tells the player why
            // they can't cant enter the room: if teleporting send false
            if(!creature->canEnter(uRoom, !teleport)) {
                (newRoom)=nullptr;
                return(false);
            }

            // the rest of these rules are for players only
            // exclude pets
            if(player && !creature->isPet()) {
                for(const auto& pet : player->pets) {
                    if(pet && !pet->canEnter(uRoom, !teleport)) {
                        if(!teleport)
                            player->print("%M won't follow you there.\n", pet.get());
                        newRoom=nullptr;
                        return(false);
                    }
                }
                // if they're leaving limbo
                if(!justLooking && player->getLocation() == player->getLimboRoom())
                    player->clearFlag(P_KILLED_BY_MOB);

                if(player->inUniqueRoom() && player->getUniqueRoomParent()->info.isArea("stor"))
                    gServer->saveStorage(player->getUniqueRoomParent());

            }

        }
    }


    std::shared_ptr<BaseRoom> room = (newRoom);

    // when entering the room, we may have to unmist them
    if( !justLooking &&
        player &&
        player->isEffected("mist") &&
        !player->isStaff() &&
        (room->flagIsSet(R_DISPERSE_MIST) || room->flagIsSet(R_ETHEREAL_PLANE) || room->isUnderwater()) )
    {
        if(room->isUnderwater())
            player->print("Water currents disperse your mist.\n");
        else {
            player->print("Swirling vapors disperse your mist.\n");
            player->stun(Random::get(5,8));
        }
        player->unmist();
    }

    return(true);
}


//*********************************************************************
//                      start
//*********************************************************************
// this function starts everyone moving - it finds the room, finds
// the exit, sees if they can enter, then removes them from the current
// room. the final step is completed in Move::finish

std::shared_ptr<BaseRoom> Move::start(const std::shared_ptr<Creature>& creature, cmd* cmnd, std::shared_ptr<Exit> *gExit, bool leader, std::list<std::shared_ptr<Creature>> *followers, int* numPeople, bool& roomPurged) {
    std::shared_ptr<BaseRoom> oldRoom = creature->getRoomParent(), newRoom=nullptr;
    std::shared_ptr<Exit> exit=nullptr;
    std::shared_ptr<Player> player = creature->getAsPlayer();
    std::shared_ptr<Monster>  monster = creature->getAsMonster();
    bool    sneaking=false, wasSneaking=false, mem=false;

    if(player) {
        if(!Move::canMove(player, cmnd))
            return(nullptr);
        // only players sneak
        sneaking = wasSneaking = Move::isSneaking(cmnd);

        if(sneaking && player->isStaff() && !player->flagIsSet(P_HIDDEN))
            player->setFlag(P_HIDDEN);
    }

    exit = Move::getExit(creature, cmnd);
    if(!exit) {
        creature->print("You don't see that exit.\n");
        if(monster && monster->getMaster())
            monster->getMaster()->print("Your pet doesn't see that exit.\n");
        return(nullptr);
    }

    // auto open closed but unlocked doors before checking if they can enter
    if(exit->flagIsSet(X_CLOSED) && !exit->flagIsSet(X_LOCKED)) {
        cmdOpen(player, cmnd);
    }
    if(player && !Move::canEnter(player, exit, leader))
        return(nullptr);

    if(!Move::getRoom(creature, exit, newRoom) || newRoom == nullptr)
        return(nullptr);

    // Rangers and F/T can't sneak with heavy armor on!
    if(sneaking && player && player->checkHeavyRestrict("sneak"))
        return(nullptr);

    auto aRoom = newRoom->isAreaRoom() ? newRoom->getAsAreaRoom() : nullptr;
    if(aRoom) {
        mem = aRoom->getStayInMemory();
        aRoom->setStayInMemory(true);
    }

    // we have to run a manual isFull check here because nobody is
    // in the room yet
    (*numPeople)++;
    if(!monster && !player->isStaff()) {
        if(newRoom->maxCapacity() && (newRoom->countVisPly() + *numPeople) > newRoom->maxCapacity()) {
            creature->print("That room is full.\n");
            // reset stayInMemory
            if(aRoom)
                aRoom->setStayInMemory(mem);
            return(nullptr);
        }
    }

    // were they killed by exit effect damage?
    if(exit->doEffectDamage(creature))
        return(nullptr);

    // save a copy of the exit for the track function
    if(leader && gExit != nullptr) {
        *gExit = exit;
    }


    if(player) {
        Move::update(player);
        sneaking = Move::sneak(player, sneaking);
    }
    Move::broadMove(creature, exit, cmnd, sneaking);

    // Was the area room purged?  If so we have no monsters to follow is in it so
    // no need to check that later
    int del = creature->deleteFromRoom(false);
    if(del & DEL_ROOM_DESTROYED) {
        // Room was purged and oldRoom is no longer valid
        roomPurged = true;
        exit = nullptr;
        oldRoom = nullptr;
    }

    // the leader doesn't use the list
    if(!leader)
        followers->push_back(creature);


    // sneaking means only your pet will follow you, even if you fail
    // also do this for destroyed portals
    bool portal = false;
    if(!exit)
        portal = true;
    if(!portal && player && exit->flagIsSet(X_PORTAL))
        portal = Move::usePortal(player, oldRoom, exit);

    if(exit)
        exit->checkReLock(creature, sneaking);

    if(player && (wasSneaking || portal)) {
        // TODO: Make pet unhide during failed sneak
        for(const auto& pet : player->pets) {
            if(pet && oldRoom == pet->getRoomParent())
                Move::start(pet, cmnd, nullptr, false, followers, numPeople, roomPurged);
        }
        // reset stayInMemory
        if(newRoom->isAreaRoom())
            newRoom->getAsAreaRoom()->setStayInMemory(mem);
        if(portal && oldRoom && exit)
            Move::deletePortal(oldRoom, exit, leader ? player : player->getGroupLeader(), followers);
        // portal owners close once they exit the room
        if(player && player->flagIsSet(P_PORTAL))
            Move::deletePortal(oldRoom, player->getCName(), leader ? player : player->getGroupLeader());
        return(newRoom);
    }


    Group* group = creature->getGroup();
    if(group && creature->getGroupStatus() == GROUP_LEADER && !creature->pFlagIsSet(P_DM_INVIS)) {
        auto it = group->members.begin();
        while(it != group->members.end()) {
            auto follower = it->lock();
            if(!follower) {
                it = group->members.erase(it);
                continue;
            }
            it++;
            if(follower == creature || follower->getGroupStatus() < GROUP_MEMBER)
                continue;

            if(oldRoom == follower->getRoomParent())
                Move::start(follower, cmnd, nullptr, false, followers, numPeople, roomPurged);
            if(roomPurged)
                oldRoom = nullptr;
        }
    }
    if(leader && (!group || creature->getGroupStatus() != GROUP_LEADER)) {
        for(const auto& pet : creature->pets) {
            if(oldRoom == pet->getRoomParent())
                Move::start(pet, cmnd, nullptr, false, followers, numPeople, roomPurged);
            if(roomPurged)
                oldRoom = nullptr;
        }
    }
    if(player && !sneaking && !roomPurged && oldRoom)
        Move::checkFollowed(player, exit, oldRoom, followers);

    // reset stayInMemory
    if(newRoom->isAreaRoom())
        newRoom->getAsAreaRoom()->setStayInMemory(mem);

    // portal owners close once they exit the room, but this means their group can follow
    if(player && player->flagIsSet(P_PORTAL))
        Move::deletePortal(oldRoom, player->getCName(), leader ? player : player->getGroupLeader(), followers);
    return(newRoom);
}



//*********************************************************************
//                      cmdGo
//*********************************************************************
// the base go command - it is called by the leader and causes everyone
// following to run the Move::start and Move::finish functions

int cmdGo(const std::shared_ptr<Player>& player, cmd* cmnd) {
    MapMarker oldMarker;
    std::shared_ptr<UniqueRoom> oldRoom = player->getUniqueRoomParent();
    std::shared_ptr<Exit>   exit;
    int     numPeople=0;

    if(!player->getRoomParent()) {
        player->print("You must be in a room to go anywhere!\n");
        return(0);
    }

    if(cmnd->num < 2) {
        player->print("Go where?\n");
        return(0);
    }

    // keep track of this room
    std::shared_ptr<AreaRoom> aRoom = player->getAreaRoomParent();
    std::list<std::shared_ptr<Creature>> followers;

    if(aRoom) {
        oldMarker = aRoom->mapmarker;
    }

    bool roomPurged = false;
    // this removes everyone from the room
    std::shared_ptr<BaseRoom> newRoom = nullptr;
    if(!(newRoom = Move::start(player, cmnd, &exit, true, &followers, &numPeople, roomPurged)))
        return(0);

    if(!roomPurged) {
        Move::track(oldRoom, oldMarker, exit, player, &followers);

        // remember this exit if it is hidden/concealed
        if(oldRoom && exit->isDiscoverable() && !exit->hasBeenUsedBy(player)) {
            exit->usedBy.insert(player->getId());
            oldRoom->getAsUniqueRoom()->saveToFile(1);
        }
    }

    // for display reasons, we only need to kill objects in
    // the room they're entering
    newRoom->killMortalObjectsOnFloor();

    // loadRoom is telling us the room the player used to be in is
    // a candidate for recycling
    if(newRoom->isAreaRoom() && aRoom == newRoom)
        newRoom = Move::recycle(aRoom, exit);

    // this adds everyone to the room
    Move::finish(player, newRoom, true, &followers);
    // we killed objects already, we can skip those this time around
    player->getRoomParent()->killMortalObjects(false);
    return(0);
}


//*********************************************************************
//                      cmdMove
//*********************************************************************
// This function attempts to move a player in one of the
// cardinal directions (n,s,e,w,u,d).

int cmdMove(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::string dir, str;

    player->clearFlag(P_AFK);

    // find the ordinal direction
    str = cmnd->str[0];
    if(!player->ableToDoCommand())
        return(0);

    if(str == "sw" || !strncmp(str.c_str(), "southw", 6))
        dir = "southwest";
    else if(str == "nw" || !strncmp(str.c_str(), "northw", 6))
        dir = "northwest";
    else if(str == "se" || !strncmp(str.c_str(), "southe", 6))
        dir = "southeast";
    else if(str == "ne" || !strncmp(str.c_str(), "northe", 6))
        dir = "northeast";

    /*  else if(!strcmp(str, "aft")
            strcpy(tempstr, "aft");
        else if(!strcmp(str, "fore")
            strcpy(tempstr, "fore");
        else if(!strcmp(str, "starb")
            strcpy(tempstr, "starboard");
        else if(!strcmp(str, "port")
            strcpy(tempstr, "port");  */

    else {
        switch(str[0]) {
        case 'n':
            dir = "north";
            break;
        case 's':
            dir = "south";
            break;
        case 'e':
            dir = "east";
            break;
        case 'w':
            dir = "west";
            break;
        case 'u':
            dir = "up";
            break;
        case 'd':
            dir = "down";
            break;
        case 'o':
        case 'l':
            dir = "out";
            break;
        }
    }

    // fake a "go north" type thing
    cmnd->num = 2;
    strcpy(cmnd->str[1], dir.c_str());
    cmnd->val[1] = 1;

    return(cmdGo(player, cmnd));
}


//*********************************************************************
//                      cmdFlee
//*********************************************************************
// wrapper which allows a player to flee

int cmdFlee(const std::shared_ptr<Player>& player, cmd* cmnd) {
    player->flee();
    return(0);
}


//*********************************************************************
//                      cmdOpen
//*********************************************************************
// This function allows a player to open a door if it is not already
// open and if it is not locked.

int cmdOpen(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Exit> exit=nullptr;

    player->clearFlag(P_AFK);


    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        player->print("Open what?\n");
        return(0);
    }

    if(player->flagIsSet(P_SITTING))
        player->stand();

    exit = findExit(player, cmnd);


    if(!exit) {
        player->print("You don't see that openable exit.\n");
        return(0);
    }

    if(!exit->flagIsSet(X_CLOSED) && !exit->flagIsSet(X_CLOSABLE)) {
        player->print("You don't see that openable exit.\n");
        return(0);
    }

    if(exit->flagIsSet(X_TOLL_TO_PASS) && player->getRoomParent()->getTollkeeper() && !player->isCt()) {
        player->printColor("You must pay a toll of %ld gold coins to go through the %s^x.\n", tollcost(player, exit, nullptr), exit->getCName());
        return(0);
    }

    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you open it" : "won't let you open it"))
        return(0);

    if(exit->flagIsSet(X_LOCKED) && !player->checkStaff("It's locked.\n"))
        return(0);

    exit->clearFlag(X_LOCKED);

    if(!exit->flagIsSet(X_CLOSED)) {
        player->print("It's already open.\n");
        return(0);
    }

    player->unhide();

    if(exit->isWall("wall-of-force")) {
        player->printColor("The %s^x is blocked by a wall of force.\n", exit->getCName());
        return(0);
    }
    // were they killed by exit effect damage?
    if(exit->doEffectDamage(player))
        return(0);
    
    exit->clearFlag(X_CLOSED);
    exit->ltime.ltime = time(nullptr);

    player->printColor("You open the %s^x.\n", exit->getCName());
    broadcast(player->getSock(), player->getParent(), "%M opens the %s^x.", player.get(), exit->getCName());

    Hooks::run(player, "openExit", exit, "openByCreature");

    if(!exit->getOpen().empty()) {
        if(exit->flagIsSet(X_ONOPEN_PLAYER)) {
            player->print("%s.\n", exit->getOpen().c_str());
        } else {
            broadcast((std::shared_ptr<Socket> )nullptr, player->getRoomParent(), exit->getOpen().c_str());
        }
    }

    return(0);
}

//*********************************************************************
//                      cmdClose
//*********************************************************************
// This function allows a player to close a door, if the door is not
// already closed, and if indeed it is a door.

int cmdClose(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Exit> exit=nullptr;

    player->clearFlag(P_AFK);


    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        player->print("Close what?\n");
        return(0);
    }

    if(player->flagIsSet(P_SITTING))
        player->stand();


    exit = findExit(player, cmnd);

    if(!exit) {
        player->print("You don't see that closeable exit.\n");
        return(0);
    }

    if(!exit->flagIsSet(X_CLOSABLE)) {
        player->print("You don't see that closeable exit.\n");
        return(0);
    }

    if(exit->flagIsSet(X_CLOSED)) {
        player->print("It's already closed.\n");
        return(0);
    }

    for(const auto& pIt: player->getRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (ply->inCombat()) {
                player->print("You cannot do that right now.\n");
                return (0);
            }
        }
    }

    if(exit->isWall("wall-of-force")) {
        player->printColor("The %s^x is blocked by a wall of force.\n", exit->getCName());
        return(0);
    }
    // were they killed by exit effect damage?
    if(exit->doEffectDamage(player))
        return(0);
    
    player->unhide();

    exit->setFlag(X_CLOSED);
    Hooks::run(player, "closeExit", exit, "closeByCreature");

    player->printColor("You close the %s^x.\n", exit->getCName());
    broadcast(player->getSock(), player->getParent(), "%M closes the %s^x.", player.get(), exit->getCName());

    return(0);

}

//*********************************************************************
//                      cmdUnlock
//*********************************************************************
// This function allows a player to unlock a door if they have the correct
// key, and it is locked.

unsigned short Object::getKey() const { return(keyVal); }
void Object::setKey(unsigned short k) { keyVal = k; }

int cmdUnlock(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;
    std::shared_ptr<Exit> exit=nullptr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        player->print("Unlock what?\n");
        return(0);
    }

    if(player->flagIsSet(P_SITTING))
        player->stand();


    exit = findExit(player, cmnd);

    if(!exit) {
        player->print("Unlock what?\n");
        return(0);
    }

    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you unlock it" : "won't let you unlock it"))
        return(0);

    if(!exit->flagIsSet(X_LOCKED) && !player->checkStaff("It's not locked.\n"))
        return(0);

    if(player->inJail()) {
        if(player->getUniqueRoomParent()->countVisPly() > 1) {
            player->print("You can't do that! Someone else might get out!\n");
            return(0);
        }
    }

    if(exit->flagIsSet(X_TO_STORAGE_ROOM)) {
        CatRef  sr = gConfig->getSingleProperty(player, PROP_STORAGE);
        if(!sr.id) {
            player->print("You must first purchase a storage room to unlock this exit.\n");
            return(0);
        }
    }

    if(exit->flagIsSet(X_WATCHER_UNLOCK) && (player->isWatcher() || player->isCt())) {
        player->unhide();

        exit->clearFlag(X_LOCKED);
        exit->ltime.ltime = time(nullptr);
        player->print("Click.\n");

        broadcast(player->getSock(), player->getParent(), "%M unlocks the %s^x.", player.get(), exit->getCName());

        broadcast(isWatcher, "^C%s unlocked the %s^x exit in room %s.\n",
            player->getCName(), exit->getCName(), player->getRoomParent()->fullName().c_str());

        logn("log.watchers", "%s unlocked the %s exit in room %s.\n",
            player->getCName(), exit->getCName(), player->getRoomParent()->fullName().c_str());

        return(0);
    }

    if(cmnd->num < 3) {
        player->print("Unlock it with what?\n");
        return(0);
    }

    object = player->findObject(player->getAsConstPlayer(), cmnd, 2);

    if(!object) {
        player->print("You don't have that.\n");
        return(0);
    }


    if(object->getType() == ObjectType::WAND && object->getMagicpower() == S_KNOCK)
        return(cmdUseWand(player, cmnd));
    if(object->getType() != ObjectType::KEY) {
        player->print("That's not a key.\n");
        return(0);
    }

    if(object->getShotsCur() < 1) {
        player->printColor("%O is broken.\n", object.get());
        return(0);
    }

    if(exit->isWall("wall-of-force")) {
        player->printColor("The %s^x is blocked by a wall of force.\n", exit->getCName());
        return(0);
    }
    // were they killed by exit effect damage?
    if(exit->doEffectDamage(player))
        return(0);
    
    if(!object->isKey(player->getUniqueRoomParent(), exit)) {
        player->print("Wrong key.\n");
        return(0);
    }

    player->unhide();

    exit->clearFlag(X_LOCKED);
    exit->ltime.ltime = time(nullptr);
    object->decShotsCur();

    if(object->use_output[0])
        player->print("%s\n", object->use_output);
    else
        player->print("Click.\n");
    broadcast(player->getSock(), player->getParent(), "%M unlocks the %s^x.", player.get(), exit->getCName());

    Hooks::run(player, "unlockExit", exit, "unlockByCreature");

    if(object->getShotsCur() < 1 && Unique::isUnique(object)) {
        player->delObj(object, true);
    }
    return(0);
}

//*********************************************************************
//                      cmdLock
//*********************************************************************
// This function allows a player to lock a door with the correct key.

int cmdLock(const std::shared_ptr<Player>& player, cmd* cmnd) {
    std::shared_ptr<Object> object=nullptr;
    std::shared_ptr<Exit> exit=nullptr;

    player->clearFlag(P_AFK);

    if(!player->ableToDoCommand())
        return(0);

    if(player->isMagicallyHeld(true))
        return(0);

    if(cmnd->num < 2) {
        player->print("Lock what?\n");
        return(0);
    }


    exit = findExit(player, cmnd);

    if(!exit) {
        player->print("Lock what?\n");
        return(0);
    }

    if(player->flagIsSet(P_SITTING))
        player->stand();


    const std::shared_ptr<Monster>  guard = player->getRoomParent()->getGuardingExit(exit, player);
    if(guard && !player->checkStaff("%M %s.\n", guard.get(), guard->flagIsSet(M_FACTION_NO_GUARD) ? "doesn't like you enough to let you lock it" : "won't let you lock it"))
        return(0);

    if(exit->flagIsSet(X_LOCKED)) {
        player->print("It's already locked.\n");
        return(0);
    }

    if(cmnd->num < 3) {
        player->print("Lock it with what?\n");
        return(0);
    }

    object = player->findObject(player, cmnd, 2);

    if(!object) {
        player->print("You don't have that.\n");
        return(0);
    }

    if(object->getType() != ObjectType::KEY) {
        player->printColor("%O is not a key.\n", object.get());
        return(0);
    }

    if(!exit->flagIsSet(X_LOCKABLE)) {
        player->print("You can't lock it.\n");
        return(0);
    }

    if(!exit->flagIsSet(X_CLOSED)) {
        player->print("You have to close it first.\n");
        return(0);
    }

    if(object->getShotsCur() < 1) {
        player->printColor("%O is broken.\n", object.get());
        return(0);
    }

    if(exit->isWall("wall-of-force")) {
        player->printColor("The %s^x is blocked by a wall of force.\n", exit->getCName());
        return(0);
    }
    // were they killed by exit effect damage?
    if(exit->doEffectDamage(player))
        return(0);
    
    if(!object->isKey(player->getUniqueRoomParent(), exit)) {
        player->print("Wrong key.\n");
        return(0);
    }

    player->unhide();
    exit->setFlag(X_LOCKED);

    player->print("Click.\n");
    broadcast(player->getSock(), player->getParent(), "%M locks the %s^x.", player.get(), exit->getCName());

    Hooks::run(player, "lockExit", exit, "lockByCreature");

    if(object->getShotsCur() < 1 && Unique::isUnique(object)) {
        player->delObj(object, true);
    }
    return(0);
}


//*********************************************************************
//                      getCatRef
//*********************************************************************
// we're given a string and expected to find a) the area and
// b) the room id.

void getCatRef(std::string str, CatRef& cr, const std::shared_ptr<Creature> & target) {
    cr.setDefault(target);

    // chop off the end!
    std::string::size_type pos = str.find(' ');
    if(pos != std::string::npos)
        str = str.substr(0, pos);

    pos = str.find('.');
    if(pos == std::string::npos)
        pos = str.find(':');
    if(pos != std::string::npos) {
        cr.setArea(str.substr(0, pos));
        str.erase(0, pos+1);
    }

    if(!str.empty() && isdigit(str.at(0)))
        cr.id = toNum<short>(str);
    else {
        if(!str.empty())
            cr.setArea(str);
        cr.id = -1;
    }
}

//*********************************************************************
//                      getDestination
//*********************************************************************
// we're given a string and expected to find a) a MapMarker or
// b) a CatRef. the string will start at the point we wish
// to search, but may contain extra crap at the end

void getDestination(const std::string &str, Location& l, const std::shared_ptr<Creature> & target) {
    l.mapmarker.reset();
    l.room.id = 0;
    getDestination(str, l.mapmarker, l.room, target);
}

void getDestination(std::string str, MapMarker& mapmarker, CatRef& cr, const std::shared_ptr<Creature> & target) {
    std::string::size_type pos=0;
    short area=0, x=0, y=0, z=0;
    bool found = false;

    // chop off the end!
    pos = str.find(' ', 0);
    if(pos != std::string::npos)
        str = str.substr(0, pos);

    boost::char_separator<char> sep(".");
    boost::tokenizer<boost::char_separator<char> > tokens(str, sep);
    auto it = tokens.begin();
    if(it != tokens.end() && it->length() > 0 && std::isdigit(it->at(0))) {
        area = toNum<short>(*it++);
        found = true;
    }
    if(it != tokens.end() && found) {
        // Only consider found if it's #.#
        x = toNum<short>(*it++);
    } else {
        found = false;
    }
    if(found) {
        if (it != tokens.end())
            y = toNum<short>(*it++);
        if (it != tokens.end())
            z = toNum<short>(*it++);
    }

    // 1.-10.7
    if(found) {
        mapmarker.set(area, x, y, z);
    } else
        getCatRef(str, cr, target);
}


//*********************************************************************
//                      getGuardingExit
//*********************************************************************

std::shared_ptr<Monster>  BaseRoom::getGuardingExit(const std::shared_ptr<Exit>& exit, const std::shared_ptr<const Player>& player) const {
    if(!exit->flagIsSet(X_PASSIVE_GUARD))
        return(nullptr);

    for(const auto& mons : monsters) {
        if(!mons->flagIsSet(M_PASSIVE_EXIT_GUARD))
            continue;
        if(player && mons->flagIsSet(M_FACTION_NO_GUARD) && Faction::willLetThrough(player, mons->getAsMonster()->getPrimeFaction()))
            continue;

        return(mons);
    }

    return(nullptr);
}


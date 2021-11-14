/*
 * ships.c
 *   Moving exits and such
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

#include <boost/algorithm/string/replace.hpp>       // for replace_all
#include <boost/iterator/iterator_traits.hpp>       // for iterator_value<>:...
#include <boost/lexical_cast/bad_lexical_cast.hpp>  // for bad_lexical_cast
#include <cstdlib>                                  // for atoi
#include <deque>                                    // for _Deque_iterator
#include <list>                                     // for list, operator==
#include <map>                                      // for operator==, _Rb_t...
#include <ostream>                                  // for basic_ostream::op...
#include <string>                                   // for string, allocator
#include <utility>                                  // for pair

#include "area.hpp"                                 // for MapMarker
#include "calendar.hpp"                             // for Calendar
#include "catRef.hpp"                               // for CatRef
#include "cmd.hpp"                                  // for cmd
#include "commands.hpp"                             // for getFullstrText
#include "config.hpp"                               // for Config, gConfig
#include "creatureStreams.hpp"                      // for Streamable, ColorOff
#include "flags.hpp"                                // for MAX_EXIT_FLAGS
#include "free_crt.hpp"                             // for free_crt
#include "global.hpp"                               // for CAP, NONUM
#include "location.hpp"                             // for Location
#include "move.hpp"                                 // for getString
#include "mudObjects/areaRooms.hpp"                 // for AreaRoom
#include "mudObjects/container.hpp"                 // for MonsterSet
#include "mudObjects/exits.hpp"                     // for Exit
#include "mudObjects/monsters.hpp"                  // for Monster
#include "mudObjects/players.hpp"                   // for Player
#include "mudObjects/rooms.hpp"                     // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"               // for UniqueRoom
#include "proto.hpp"                                // for broadcast, zero
#include "random.hpp"                               // for Random
#include "range.hpp"                                // for Range
#include "server.hpp"                               // for Server, gServer
#include "ships.hpp"                                // for ShipStop, ShipRaid
#include "utils.hpp"                                // for MAX
#include "wanderInfo.hpp"                           // for WanderInfo
#include "xml.hpp"                                  // for copyToString, sav...


#define PIRATE_QUEST (54 - 1)



// Notes:
// If you mess up the data up, you'll never know what time the ship
// will be at the docks. You will need to check that ALL of the time
// spent at stops / time spent traveling adds up to some 24 hour
// increment or divides into 24.
// For example, if it adds up to 25, you'll get:
//      stop 1, day 1: 8am
//      stop 1, day 2: 9am
//      stop 1, day 3: 10am


//*********************************************************************
//                      shipBroadcastRange
//*********************************************************************
// given a ShipStop and a message, we'll announce it to the
// mud (appropriate rooms)

bool ShipStop::belongs(const CatRef& cr) {
    std::list<Range>::iterator rt;
    for(rt = announce.begin() ; rt != announce.end() ; rt++)
        if((*rt).belongs(cr))
            return(true);
    return(false);
}

bool Ship::belongs(const CatRef& cr) {
    std::list<Range>::iterator rt;
    for(rt = ranges.begin() ; rt != ranges.end() ; rt++)
        if((*rt).belongs(cr))
            return(true);
    return(false);
}

void shipBroadcastRange(Ship *ship, ShipStop *stop, const std::string& message) {

    if(message.empty())
        return;

    // if our announce range isnt there, announce to everyone and quit
    if(stop->announce.empty()) {
        broadcast("%s", message.c_str());
        return;
    }

    Player* target=nullptr;
    // otherwise go through each player
    for(const auto& p : gServer->players) {
        target = p.second;

        if(!target->isConnected())
            continue;
        if(!target->inUniqueRoom())
            continue;

        if( ship->belongs(target->getUniqueRoomParent()->info) ||
            stop->belongs(target->getUniqueRoomParent()->info)
        )
            target->print("%s\n", message.c_str());
    }
}



//*********************************************************************
//                      spawnRaiders
//*********************************************************************
// run through the exits declared as Raid and spawn some raiders!

void ShipExit::spawnRaiders(ShipRaid* sRaid) {
    Monster *raider=nullptr;
    BaseRoom *room=nullptr;
    int     l=0, total=0;

    if(name.empty() || !raid || !origin.getId() || !target.getId())
        return;

    if(!loadMonster(sRaid->getSearchMob(), &raider))
        return;

    room = getRoom(false);
    if(!room) {
        free_crt(raider);
        return;
    }

    if(!raider->flagIsSet(M_CUSTOM))
        raider->validateAc();

    if(!raider->getName()[0] || raider->getName()[0] == ' ') {
        free_crt(raider);
        return;
    }

    total = Random::get(sRaid->getMinSpawnNum(), sRaid->getMaxSpawnNum());

    for(l=0; l<total;) {
        raider->initMonster(true, false);

        if(!l)
            raider->addToRoom(room, total);
        else
            raider->addToRoom(room, 0);

        raider->setFlag(M_PERMENANT_MONSTER);
        raider->setFlag(M_RAIDING);
        gServer->addActive(raider);
        l++;
        if(l < total)
            loadMonster(sRaid->getSearchMob(), &raider);
    }
}


//*********************************************************************
//                      getRoom
//*********************************************************************

BaseRoom* ShipExit::getRoom(bool useOrigin) {
    if(useOrigin)
        return(origin.loadRoom());
    return(target.loadRoom());
}


//*********************************************************************
//                      spawnRaiders
//*********************************************************************
// run through the exits declared as Raid and spawn some raiders!

void ShipStop::spawnRaiders() {
    std::list<ShipExit*>::iterator xt;

    if(!raid || !raid->getSearchMob())
        return;

    for(xt = exits.begin() ; xt != exits.end() ; xt++)
        (*xt)->spawnRaiders(raid);
}


//*********************************************************************
//                      createExit
//*********************************************************************
// creates the exits for the ShipExit it's given
// it also takes care of announcing the ship's arrival

bool ShipExit::createExit() {
    BaseRoom* newRoom=nullptr;
    int     i=0;

    if(name.empty() || !origin.getId() || !target.getId())
        return(false);

    newRoom = getRoom(true);
    if(!newRoom)
        return(false);

    link_rom(newRoom, target, name);

    for(Exit* ext : newRoom->exits) {
        if(ext->getName() == name) {
            zero(ext->flags, sizeof(ext->flags));

            // TODO: Dom: needs to use getFlags() and setFlags()
            for(i=sizeof(ext->flags)-1; i>=0; i--)
                ext->flags[i] = flags[i];
            ext->setFlag(X_MOVING);

            if(!arrives.empty())
                broadcast(nullptr, newRoom, "%s", arrives.c_str());
            return(true);
        }
    }

    return(false);
}

// hand on that responsibility!
// but do broadcasting and raider-spawning
int shipSetExits(Ship *ship, ShipStop *stop) {
    std::list<ShipExit*>::iterator exit;
    bool        broad = false;

    for(exit = stop->exits.begin() ; exit != stop->exits.end() ; exit++)
        broad = (*exit)->createExit() || broad;

    if(broad)
        shipBroadcastRange(ship, stop, stop->arrives);
    stop->spawnRaiders();
    return(0);
}



//*********************************************************************
//                      removeExit
//*********************************************************************
// Given a ship exits, it will delete the appropriate moving exits.
// Will also cause raiders to wander away.

void ShipExit::removeExit() {
    Monster *raider=nullptr;
    BaseRoom* newRoom=nullptr;
    AreaRoom* aRoom=nullptr;
    int     i=0, n=0;

    newRoom = getRoom(true);
    if(!newRoom)
        return;

    // kill all raiders
    auto mIt = newRoom->monsters.begin();
    while(mIt != newRoom->monsters.end()) {
        raider = (*mIt++);

        if(raider && raider->flagIsSet(M_RAIDING)) {
            broadcast(nullptr, newRoom, "%1M just %s away.", raider, Move::getString(raider).c_str());
            gServer->delActive(raider);
            raider->deleteFromRoom();
            raider->clearAsEnemy();
            free_crt(raider);
        }
    }


    if(!departs.empty())
        broadcast(nullptr, newRoom, "%s", departs.c_str());

    aRoom = newRoom->getAsAreaRoom();
    ExitList::iterator xit;
    for(xit = newRoom->exits.begin() ; xit != newRoom->exits.end() ; ) {
        Exit* ext = (*xit++);
        // if we're given an exit, delete it
        // otherwise delete all exits
        if(ext->flagIsSet(X_MOVING) && ext->getName() == name) {
            //oldPrintColor(4, "^rDeleting^w exit %s in room %d.\n", xt->name, room);
            if(i < 8 && aRoom) {
                // it's a cardinal direction, clear all flags
                for(n=0; n<MAX_EXIT_FLAGS; n++)
                    ext->clearFlag(n);
                ext->target.room.id = 0;
                aRoom->updateExits();
            } else
                newRoom->delExit(ext);
        }
        i++;
    }
}

// hand on that responsibility!
// but do broadcasting and kicking people off a raiding vessel
int shipDeleteExits(Ship *ship, ShipStop *stop) {
    std::list<ShipExit*>::iterator exit;
    Monster     *raider=nullptr;
    UniqueRoom      *room=nullptr, *newRoom=nullptr;
    int         found=1;

    if(stop->raid) {
        // record when the last pirate raid was
        if(stop->raid->getRecord())
            gConfig->calendar->setLastPirate(ship->name);

        // kick people off the ship!
        if( stop->raid->getSearchMob() && (stop->raid->getDump().id || stop->raid->getPrison().id) && loadMonster(stop->raid->getSearchMob(), &raider)) {
            // otherwise go through each player
            for(const auto& [pId, ply] : gServer->players) {

                if(!ply->isConnected())
                    continue;
                if(ply->isStaff())
                    continue;
                if(!ply->inUniqueRoom())
                    continue;

                if(!ship->belongs(ply->getUniqueRoomParent()->info))
                    continue;


                if(!loadRoom(ply->getUniqueRoomParent()->info, &room))
                    continue;
                if(!room->wander.getTraffic())
                    continue;

                found = 1;


                *ply << ColorOn << raider->getCrtStr(nullptr, CAP, 1) << " just arrived\n" << ColorOff;
                ply->sendPrompt();

                // you can't see me!
                if(!raider->canSee(ply))
                    found = 0;

                // if they're hidden - look for them!
                if(found && ply->flagIsSet(P_HIDDEN)) {
                    found = 0;

                    *ply << ColorOn << raider->getCrtStr(nullptr, CAP | NONUM, 0) << " searches the room.\n" << ColorOff;

                    if(Random::get(1,100) <= SHIP_SEARCH_CHANCE) {
                        found = 1;
                        ply->unhide();

                        *ply << raider->upHeShe() <<" found something!\n";
                    }
                    ply->sendPrompt();
                }

                if(!found) {
                    *ply << ColorOn<< raider->getCrtStr(nullptr, CAP|NONUM, 0) << " wanders away.\n" << ColorOff;
                    ply->sendPrompt();
                    continue;
                }

                ply->wake("You awaken suddenly!");

                if( stop->raid->getDump().id && !(ply->isUnconscious() && stop->raid->getUnconInPrison()) && loadRoom(stop->raid->getDump(), &newRoom)) {
                    if(!stop->raid->getDumpTalk().empty()) {
                        *ply << ColorOn << raider->getCrtStr(nullptr, CAP|NONUM, 0) << " says to you, \"" << stop->raid->getDumpTalk() << "\".\n" << ColorOff;
                    }
                    if(!stop->raid->getDumpAction().empty()) {
                        if(!stop->raid->getDumpTalk().empty())
                            ply->sendPrompt();
                        std::string tmp = stop->raid->getDumpAction();
                        std::string tmp2 = raider->getCrtStr(nullptr, CAP|NONUM, 0);
                        boost::replace_all(tmp, "*ACTOR*", tmp2.c_str());
                        *ply << ColorOn << tmp << "\n" << ColorOff;
                    }
                    ply->deleteFromRoom();
                    ply->addToRoom(newRoom);
                    ply->doPetFollow();

                } else if(stop->raid->getPrison().id && loadRoom(stop->raid->getPrison(), &newRoom)) {

                    if(!stop->raid->getPrisonTalk().empty()) {
                        *ply << ColorOn << raider->getCrtStr(nullptr, CAP|NONUM, 0) << "says to you \"" << stop->raid->getPrisonTalk() + "\".\n" << ColorOff;
                    }
                    if(!stop->raid->getPrisonAction().empty()) {
                        if(!stop->raid->getPrisonTalk().empty())
                            ply->sendPrompt();
                        std::string tmp = stop->raid->getPrisonAction();
                        std::string tmp2 = raider->getCrtStr(nullptr, CAP|NONUM, 0);
                        boost::replace_all(tmp, "*ACTOR*", tmp2.c_str());
                        *ply << ColorOn << tmp << "\n" << ColorOff;
                    }
                    ply->deleteFromRoom();
                    ply->addToRoom(newRoom);
                    ply->doPetFollow();
                }

                broadcast(nullptr, room, "%M was hauled off by %N.", ply, raider);
            }
            free_crt(raider);
        }
    }

    for(exit = stop->exits.begin() ; exit != stop->exits.end() ; exit++)
        (*exit)->removeExit();

    shipBroadcastRange(ship, stop, stop->departs);
    return(0);
}




// ___...---...___...---...___...---
//      ShipRaid
// ___...---...___...---...___...---

ShipRaid::ShipRaid() {
    searchMob = raidMob = minSpawnNum = maxSpawnNum = record = false;
    dumpTalk = prisonTalk = dumpAction = prisonAction = "";
    unconInPrison = false;
}

void ShipRaid::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Record")) xml::copyToBool(record, childNode);
        else if(NODE_NAME(childNode, "SearchMob")) xml::copyToNum(searchMob, childNode);
        else if(NODE_NAME(childNode, "RaidMob")) xml::copyToNum(raidMob, childNode);
        else if(NODE_NAME(childNode, "MinSpawnNum")) xml::copyToNum(minSpawnNum, childNode);
        else if(NODE_NAME(childNode, "MaxSpawnNum")) xml::copyToNum(maxSpawnNum, childNode);
        else if(NODE_NAME(childNode, "Dump")) dump.load(childNode);
        else if(NODE_NAME(childNode, "Prison")) prison.load(childNode);
        else if(NODE_NAME(childNode, "DumpTalk")) xml::copyToString(dumpTalk, childNode);
        else if(NODE_NAME(childNode, "PrisonTalk")) xml::copyToString(prisonTalk, childNode);
        else if(NODE_NAME(childNode, "DumpAction")) xml::copyToString(dumpAction, childNode);
        else if(NODE_NAME(childNode, "PrisonAction")) xml::copyToString(prisonAction, childNode);
        else if(NODE_NAME(childNode, "UnconInPrison")) xml::copyToBool(unconInPrison, childNode);

        childNode = childNode->next;
    }
    minSpawnNum = MAX(0, minSpawnNum);
    if(maxSpawnNum < minSpawnNum)
        maxSpawnNum = minSpawnNum;
}

void ShipRaid::save(xmlNodePtr curNode) const {
    xml::saveNonZeroNum(curNode, "Record", record);
    xml::saveNonZeroNum(curNode, "SearchMob", searchMob);
    xml::saveNonZeroNum(curNode, "RaidMob", raidMob);
    xml::saveNonZeroNum(curNode, "MinSpawnNum", minSpawnNum);
    xml::saveNonZeroNum(curNode, "MaxSpawnNum", maxSpawnNum);

    dump.save(curNode, "Dump", false);
    prison.save(curNode, "Prison", false);
    xml::saveNonNullString(curNode, "DumpTalk", dumpTalk);
    xml::saveNonNullString(curNode, "PrisonTalk", prisonTalk);
    xml::saveNonNullString(curNode, "DumpAction", dumpAction);
    xml::saveNonNullString(curNode, "PrisonAction", prisonAction);

    xml::saveNonZeroNum(curNode, "UnconInPrison", unconInPrison);
}

// get functions
int ShipRaid::getRecord() const {
    return(record);
}
int ShipRaid::getSearchMob() const {
    return(searchMob);
}
int ShipRaid::getRaidMob() const {
    return(raidMob);
}
int ShipRaid::getMinSpawnNum() const {
    return(minSpawnNum);
}
int ShipRaid::getMaxSpawnNum() const {
    return(maxSpawnNum);
}
CatRef ShipRaid::getDump() const {
    return(dump);
}
CatRef ShipRaid::getPrison() const {
    return(prison);
}
std::string ShipRaid::getDumpTalk() const {
    return(dumpTalk);
}
std::string ShipRaid::getPrisonTalk() const {
    return(prisonTalk);
}
std::string ShipRaid::getDumpAction() const {
    return(dumpAction);
}
std::string ShipRaid::getPrisonAction() const {
    return(prisonAction);
}
bool ShipRaid::getUnconInPrison() const {
    return(unconInPrison);
}


// ___...---...___...---...___...---
//      ShipExit
// ___...---...___...---...___...---

ShipExit::ShipExit() {
    raid = false;
    zero(flags, sizeof(flags));
    name = "";
    arrives = "";
    departs = "";
}

std::string ShipExit::getName() const { return(name); }
bool ShipExit::getRaid() const { return(raid); }
std::string ShipExit::getArrives() const { return(arrives); }
std::string ShipExit::getDeparts() const { return(departs); }
const char *ShipExit::getFlags() const { return(flags); }

void ShipExit::setFlags(char f) {
    int i = sizeof(flags);
    char tmp[i];
    zero(tmp, i);
    *(tmp) = f;
    for(i--; i>=0; i--)
        flags[i] = tmp[i];
}


void ShipExit::save(xmlNodePtr curNode) const {
    xml::saveNonNullString(curNode, "Name", name);
    origin.save(curNode, "Origin");
    target.save(curNode, "Target");
    saveBits(curNode, "Flags", MAX_EXIT_FLAGS, flags);
    xml::saveNonNullString(curNode, "Arrives", arrives);
    xml::saveNonNullString(curNode, "Departs", departs);
    xml::saveNonZeroNum(curNode, "Raid", raid);
}

void ShipExit::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) { xml::copyToString(name, childNode); }
        else if(NODE_NAME(childNode, "Origin")) origin.load(childNode);
        else if(NODE_NAME(childNode, "Target")) target.load(childNode);
        else if(NODE_NAME(childNode, "Arrives")) xml::copyToString(arrives, childNode);
        else if(NODE_NAME(childNode, "Departs")) xml::copyToString(departs, childNode);
        else if(NODE_NAME(childNode, "Flags")) loadBits(childNode, flags);
        else if(NODE_NAME(childNode, "Raid")) raid = true;
        childNode = childNode->next;
    }
}

// ___...---...___...---...___...---
//      ShipStop
// ___...---...___...---...___...---

ShipStop::ShipStop() {
    name = arrives = lastcall = departs = "";
    to_next = in_dock = 0;
    raid = nullptr;
}

ShipStop::~ShipStop() {
    ShipExit* exit=nullptr;

    delete raid;

    while(!exits.empty()) {
        exit = exits.front();
        delete exit;
        exits.pop_front();
    }
    exits.clear();
}

void ShipStop::load(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;

    while(childNode) {
        if(NODE_NAME(childNode, "Name")) { xml::copyToString(name, childNode); }
        else if(NODE_NAME(childNode, "Arrives")) { xml::copyToString(arrives, childNode); }
        else if(NODE_NAME(childNode, "LastCall")) { xml::copyToString(lastcall, childNode); }
        else if(NODE_NAME(childNode, "Departs")) { xml::copyToString(departs, childNode); }

        else if(NODE_NAME(childNode, "ToNext")) xml::copyToNum(to_next, childNode);
        else if(NODE_NAME(childNode, "InDock")) xml::copyToNum(in_dock, childNode);

        else if(NODE_NAME(childNode, "Raid")) {
            raid = new ShipRaid;
            raid->load(childNode);
        }
        else if(NODE_NAME(childNode, "Exits"))
            loadExits(childNode);

        else if(NODE_NAME(childNode, "AnnounceRange")) {
            xmlNodePtr subNode = childNode->children;

            while(subNode) {
                if(NODE_NAME(subNode, "Range")) {
                    Range r;
                    r.load(subNode);
                    announce.push_back(r);
                }
                subNode = subNode->next;
            }
        }
        childNode = childNode->next;
    }
}

void ShipStop::loadExits(xmlNodePtr curNode) {
    xmlNodePtr childNode = curNode->children;
    ShipExit    *exit=nullptr;

    while(childNode) {
        if(NODE_NAME(childNode, "Exit")) {
            exit = new ShipExit;
            exit->load(childNode);
            if(exit->origin.room.id || exit->origin.mapmarker.getArea())
                exit->removeExit();
            exits.push_back(exit);
        }
        childNode = childNode->next;
    }
}

void ShipStop::save(xmlNodePtr rootNode) const {
    xmlNodePtr  curNode, childNode, xchildNode;

    curNode = xml::newStringChild(rootNode, "Stop");
    xml::saveNonNullString(curNode, "Name", name);
    xml::saveNonZeroNum(curNode, "InDock", in_dock);
    xml::saveNonZeroNum(curNode, "ToNext", to_next);

    xml::saveNonNullString(curNode, "Arrives", arrives);
    xml::saveNonNullString(curNode, "LastCall", lastcall);
    xml::saveNonNullString(curNode, "Departs", departs);

    if(raid) {
        childNode = xml::newStringChild(curNode, "Raid");
        raid->save(childNode);
    }

    childNode = xml::newStringChild(curNode, "Exits");

    std::list<ShipExit*>::const_iterator exit;
    for(exit = exits.begin() ; exit != exits.end() ; exit++) {
        xchildNode = xml::newStringChild(childNode, "Exit");
        (*exit)->save(xchildNode);
    }

    if(!announce.empty()) {
        childNode = xml::newStringChild(curNode, "AnnounceRange");
        std::list<Range>::const_iterator rt;
        for(rt = announce.begin() ; rt != announce.end() ; rt++)
            (*rt).save(childNode, "Range");
    }
}

// ___...---...___...---...___...---
//      Ship
// ___...---...___...---...___...---

Ship::Ship() {
    timeLeft = clan = 0;
    name = transit = movement = docked = "";
    inPort = canQuery = false;
}

Ship::~Ship() {
    ShipStop* stop=nullptr;

    while(!stops.empty()) {
        stop = stops.front();
        delete stop;
        stops.pop_front();
    }
    stops.clear();
}

//*********************************************************************
//                      shipPrintRange
//*********************************************************************

// used in dmQueryShips to print out the ranges

void shipPrintRange(Player* player, std::list<Range>* list) {
    std::list<Range>::iterator rt;
    for(rt = list->begin() ; rt != list->end() ; rt++)
        player->print("  %s\n", (*rt).str().c_str());
}

//*********************************************************************
//                      dmQueryShips
//*********************************************************************
// dm command to show all information about ships

int dmQueryShips(Player* player, cmd* cmnd) {
    int         mod=0, id=0;
    std::list<Ship*>::iterator it;
    std::list<ShipStop*>::iterator st;
    std::list<ShipExit*>::iterator xt;
    Ship        *ship=nullptr;
    ShipStop    *stop=nullptr;
    ShipExit    *exit=nullptr;
    int shipID = atoi(getFullstrText(cmnd->fullstr, 1).c_str());
    int stopID = atoi(getFullstrText(cmnd->fullstr, 2).c_str());
    const Calendar* calendar = nullptr;

    if(!shipID) {
        calendar = gConfig->getCalendar();
        player->print("Location of All Ships:\n");
        player->printColor("^b-------------------------------------------------------------------------------\n");

        id = 1;
        for(it = gConfig->ships.begin() ; it != gConfig->ships.end() ; it++) {
            ship = (*it);
            player->print("  %-25s %s", ship->name.c_str(), ship->inPort ? "Stopped at " : "In transit, going to ");

            st = ship->stops.begin();
            stop = (*st);
            player->print("%s.\n     #%-25dWill ", stop->name.c_str(), id);
            if(ship->inPort) {
                st++;
                stop = (*st);
                player->print("depart for %s", stop->name.c_str());
            } else {
                player->print("arrive");
            }
            player->print(" in ");
            mod = ship->timeLeft / 60;
            if(mod)
                player->print("%d hour%s%s", mod, mod==1 ? "" : "s", ship->timeLeft % 60 ? " " : "");
            mod = ship->timeLeft % 60;
            if(mod)
                player->print("%d minute%s", mod, mod==1 ? "" : "s");
            player->print(".\n");

            id++;
        }
        player->print("\nType \"*ship [ship id]\" to view the details of a specific ship.\n");
        player->printColor("Ship Updates:  ^c%d^x / ^c%d^x    Hour: ^c%d^x / ^c%d^x.\n",
            calendar->shipUpdates, gConfig->expectedShipUpdates(),
            calendar->shipUpdates % 60, gConfig->expectedShipUpdates() % 60);
        player->print("The last pirate raid was: %s\n\n", calendar->getLastPirate().c_str());
        return(0);
    }


    mod = 1;
    // find the ship we're looking at
    for(it = gConfig->ships.begin() ; it != gConfig->ships.end() && mod != shipID ; it++)
        mod++;
    ship = (*it);

    if(!ship) {
        player->print("Invalid ship!\n");
        return(0);
    }

    player->print("Information on %s:\n", ship->name.c_str());
    player->printColor("^b-------------------------------------------------------------------------------\n");

    stop = ship->stops.front();
    player->print("In Port:   %s\n", ship->inPort ? "Yes" : "No");
    player->print("Time Left: %d minutes\n", ship->timeLeft);

    if(!stopID) {

        player->print("Can Query: %s\n", ship->canQuery ? "Yes" : "No");
        if(!ship->transit.empty())
            player->print("Transit:   %s\n", ship->transit.c_str());
        if(!ship->movement.empty())
            player->print("Movement:  %s\n", ship->movement.c_str());
        if(!ship->docked.empty())
            player->print("Docked:    %s\n", ship->docked.c_str());
        player->print("\n");

        // boat range
        player->print("Boat Ranges:\n");
        shipPrintRange(player, &ship->ranges);
        player->print("\n");

        player->print("Stops:\n");

        id = 1;

        for(st = ship->stops.begin() ; st != ship->stops.end() ; st++) {
            stop = (*st);
            player->print("-----------------------\n");
            player->print("  Stop: %d - %s\n", id, stop->name.c_str());
            player->print("  Raid: %s\n", stop->raid ? "Yes" : "No");

            player->print("  Exits:\n");
            mod = 1;
            for(xt = stop->exits.begin() ; xt != stop->exits.end() ; xt++) {
                exit = (*xt);
                player->print("     Exit: %d - %s, room %s %s\n", mod, exit->getName().c_str(),
                    exit->origin.room.str().c_str(), exit->origin.mapmarker.getArea() ? exit->origin.mapmarker.str().c_str() : "");
                mod++;
            }

            player->print("\n");
            id++;
        }

        player->print("Type \"*ship [ship id] [stop id]\" to view the details of a specific stop.\n\n");

    } else {

        mod = 1;
        // find the stop we're looking at
        for(st = ship->stops.begin() ; st != ship->stops.end() && mod != stopID ; st++)
            mod++;

        stop = (*st);

        if(!stop) {
            player->print("Invalid stop!\n");
            return(0);
        }

        player->print("Stop: %d - %s\n", id, stop->name.c_str());
        player->print("In Dock:   %d minutes\n", stop->in_dock);
        player->print("To Next:   %d minutes\n\n", stop->to_next);

        if(!stop->arrives.empty())
            player->print("Arrives:   %s\n", stop->arrives.c_str());
        if(!stop->lastcall.empty())
            player->print("Last Call: %s\n", stop->lastcall.c_str());
        if(!stop->departs.empty())
            player->print("Departs:   %s\n", stop->departs.c_str());
        if(!stop->arrives.empty() || !stop->lastcall.empty() || !stop->departs.empty())
            player->print("\n");

        if(stop->raid) {
            player->print("Raid Info:\n-----------------------\n");
            player->print("Record:          %s\n", stop->raid->getRecord() ? "Yes" : "No");
            player->print("Search Mob:      %d\n", stop->raid->getSearchMob());
            player->print("Dump Room:       %s\n", stop->raid->getDump().str().c_str());
            player->print("Prison Room:     %s\n", stop->raid->getPrison().str().c_str());
            if(!stop->raid->getDumpTalk().empty())
                player->print("Dump Talk:       %s\n", stop->raid->getDumpTalk().c_str());
            if(!stop->raid->getPrisonTalk().empty())
                player->print("Prison Talk:     %s\n", stop->raid->getPrisonTalk().c_str());
            if(!stop->raid->getDumpAction().empty())
                player->print("Dump Action:     %s\n", stop->raid->getDumpAction().c_str());
            if(!stop->raid->getPrisonAction().empty())
                player->print("Prison Action:   %s\n", stop->raid->getPrisonAction().c_str());
            player->print("Uncon In Prison: %s\n\n", stop->raid->getUnconInPrison() ? "Yes" : "No");
        }

        // announce range
        player->print("Announce Ranges:\n");
        shipPrintRange(player, &stop->announce);
        player->print("\n");

        player->print("Exits:\n");
        player->print("-----------------------\n");

        mod = 1;
        for(xt = stop->exits.begin() ; xt != stop->exits.end() ; xt++) {
            exit = (*xt);
            player->print("  Exit:    %d - %s\n", mod, exit->getName().c_str());
            player->print("  uOrigin: %s   aOrigin: %s\n", exit->origin.room.str().c_str(),
                exit->origin.mapmarker.str().c_str());
            player->print("  uTarget: %s   aTarget: %s\n", exit->target.room.str().c_str(),
                exit->target.mapmarker.str().c_str());
            player->print("  Raid:    %s\n", exit->getRaid() ? "Yes" : "No");
            if(!exit->getArrives().empty())
                player->print("  Arrives: %s\n", exit->getArrives().c_str());
            if(!exit->getDeparts().empty())
                player->print("  Departs: %s\n", exit->getDeparts().c_str());

            player->print("\n");
            mod++;
        }
    }

    return(0);
}


//*********************************************************************
//                      cmdQueryShips
//*********************************************************************
// let the player see where ships are

int cmdQueryShips(Player* player, cmd* cmnd) {
    int     mod=0;
    std::list<Ship*>::iterator it;
    std::list<ShipStop*>::iterator st;
    Ship    *ship=nullptr;
    ShipStop *stop=nullptr;

    if(!player->ableToDoCommand())
        return(0);

    player->print("Location of Chartered Ships:\n");
    player->printColor("^b-------------------------------------------------------------------------------\n");

    for(it = gConfig->ships.begin() ; it != gConfig->ships.end() ; it++) {
        ship = (*it);
        // the wizard's eye tower can be viewed by clan members
        if(ship->canQuery || (ship->clan && ship->clan == player->getClan())) {
            player->print("  %-25s ", ship->name.c_str());
            if(ship->inPort)
                player->print("%s at ", ship->docked.c_str());
            else
                player->print("%s, %s to ", ship->transit.c_str(), ship->movement.c_str());

            st = ship->stops.begin();
            stop = (*st);
            player->print("%s.\n%-31sWill ", stop->name.c_str(), " ");
            if(ship->inPort) {
                st++;
                stop = (*st);
                player->print("depart for %s", stop->name.c_str());
            } else
                player->print("arrive");

            player->print(" in ");
            mod = ship->timeLeft / 60;
            if(mod)
                player->print("%d hour%s%s", mod, mod==1 ? "" : "s", ship->timeLeft % 60 ? " " : "");
            mod = ship->timeLeft % 60;
            if(mod)
                player->print("%d minute%s", mod, mod==1 ? "" : "s");
            player->print(".\n");
        }
    }

    // Dont let staff cheat and set the quest on themselves
    // to see where the Marauder will be.
    if(player->isStaff() && !player->isDm())
        return(0);

    if(player->questIsSet(PIRATE_QUEST) || player->isDm())
        player->print("\nThe last pirate raid was: %s\n", gConfig->getCalendar()->getLastPirate().c_str());

    return(0);
}

//*********************************************************************
//                      clearShips
//*********************************************************************

void Config::clearShips() {
    Ship    *ship=nullptr;

    while(!ships.empty()) {
        ship = ships.front();
        delete ship;
        ships.pop_front();
    }
    ships.clear();
}

/*
 * move.h
 *   Namespace for movement.
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

#include <list>


class cmd;
class AreaRoom;
class BaseRoom;
class Creature;
class Container;
class Exit;
class MapMarker;
class Player;
class UniqueRoom;

namespace Move {
    bool tooFarAway(const std::shared_ptr<BaseRoom>& pRoom, const std::shared_ptr<BaseRoom>& tRoom, bool track);
    bool tooFarAway(const std::shared_ptr<Creature>& player, const std::shared_ptr<BaseRoom>& room);
    bool tooFarAway(const std::shared_ptr<Creature>& player, const std::shared_ptr<Creature>& target, const std::string& action);
    void broadcast(const std::shared_ptr<Creature>& player, const std::shared_ptr<const Container>& container, bool ordinal, const std::string& exit, bool hiddenExit);
    std::string formatFindExit(cmd* cmnd);
    bool isSneaking(cmd* cmnd);
    bool isOrdinal(cmd* cmnd);
    std::shared_ptr<AreaRoom> recycle(std::shared_ptr<AreaRoom> room, const std::shared_ptr<Exit>& exit);
    std::shared_ptr<UniqueRoom> getUniqueRoom(std::shared_ptr<Creature> creature, std::shared_ptr<Player> player, std::shared_ptr<Exit> exit, MapMarker* tMapmarker);
    void update(const std::shared_ptr<Player>& player);
    void broadMove(const std::shared_ptr<Creature>& player, const std::shared_ptr<Exit>& exit, cmd* cmnd, bool sneaking);
    bool track(const std::shared_ptr<UniqueRoom>& room, const MapMarker& mapmarker, const std::shared_ptr<Exit>& exit, const std::shared_ptr<Player>& player, bool reset);
    void track(const std::shared_ptr<UniqueRoom>& room, const MapMarker& mapmarker, const std::shared_ptr<Exit>& exit, const std::shared_ptr<Player>&leader, std::list<std::shared_ptr<Creature>> *followers);
    bool sneak(const std::shared_ptr<Player>& player, bool sneaking);
    bool canEnter(const std::shared_ptr<Player>& player, const std::shared_ptr<Exit>& exit, bool leader);
    bool canMove(const std::shared_ptr<Player>& player, cmd* cmnd);
    std::shared_ptr<Exit> getExit(std::shared_ptr<Creature> player, cmd* cmnd);
    std::string getString(const std::shared_ptr<Creature>& creature, bool ordinal=false, std::string_view exit = "");
    void checkFollowed(const std::shared_ptr<Player>& player, const std::shared_ptr<Exit>& exit, const std::shared_ptr<BaseRoom>& room, std::list<std::shared_ptr<Creature>> *followers);
    void finish(const std::shared_ptr<Creature>& creature, const std::shared_ptr<BaseRoom>& room, bool self, std::list<std::shared_ptr<Creature>> *followers);
    bool getRoom(const std::shared_ptr<Creature>& creature, const std::shared_ptr<Exit>& exit, std::shared_ptr<BaseRoom>& newRoom, bool justLooking=false, MapMarker* tMapmarker=0, bool recycle=true);
    std::shared_ptr<BaseRoom> start(const std::shared_ptr<Creature>& creature, cmd* cmnd, std::shared_ptr<Exit> *gExit, bool leader, std::list<std::shared_ptr<Creature>> *followers, int* numPeople, bool& roomPurged);

    void createPortal(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<BaseRoom>& target, const std::shared_ptr<Player>& player, bool initial=true);
    bool usePortal(const std::shared_ptr<Creature> &creature, const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Exit>& exit, bool initial=true);
    bool deletePortal(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Exit>& exit, const std::shared_ptr<Creature> & leader=0, std::list<std::shared_ptr<Creature>> *followers=0, bool initial=true);
    bool deletePortal(const std::shared_ptr<BaseRoom>& room, const std::string &name, const std::shared_ptr<Creature> & leader=0, std::list<std::shared_ptr<Creature>> *followers=0, bool initial=true);
}


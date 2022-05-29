/*
 * anchor.h
 *   Magical anchors
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

#include "area.hpp"
#include "catRef.hpp"

class AreaRoom;
class BaseRoom;
class Player;
class UniqueRoom;

class Anchor {
public:
    Anchor();
    Anchor(std::string_view a, const std::shared_ptr<Player>& player);
    ~Anchor();
    void reset();

    Anchor& operator=(const Anchor& a);
    void load(xmlNodePtr curNode);
    void save(xmlNodePtr curNode) const;

    void bind(const std::shared_ptr<const Player> &player);
    void bind(const std::shared_ptr<const UniqueRoom> &uRoom);
    void bind(const std::shared_ptr<const AreaRoom> &aRoom);
    void setRoom(const CatRef& r);

    [[nodiscard]] bool is(const std::shared_ptr<const BaseRoom> &pRoom) const;
    [[nodiscard]] bool is(const std::shared_ptr<const Player> &player) const;
    [[nodiscard]] bool is(const std::shared_ptr<const UniqueRoom> &uRoom) const;
    [[nodiscard]] bool is(const std::shared_ptr<const AreaRoom> &aRoom) const;

    [[nodiscard]] std::string getAlias() const;
    [[nodiscard]] std::string getRoomName() const;
    [[nodiscard]] CatRef getRoom() const;
    [[nodiscard]] const MapMarker& getMapMarker() const;
protected:
    std::string alias;
    std::string roomName;
    CatRef room;
    bool _hasMarker = false;
public:
    bool hasMarker() const;
protected:
    MapMarker mapmarker{};
};


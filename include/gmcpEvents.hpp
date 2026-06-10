/*
 * gmcpEvents.hpp
 *   GMCP live event deltas pushed from game mutation sites
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

#include <memory>
#include <string>
#include <string_view>

#include <nlohmann/json_fwd.hpp>

class BaseRoom;
class EffectInfo;
class MudObject;
class Object;
class Player;

namespace gmcp {
    void onInventoryAdd(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>& obj);
    void onInventoryRemove(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>& obj);
    void onRoomItemAdd(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Object>& obj);
    void onRoomItemRemove(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Object>& obj);

    void onRoomEnter(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Player>& who);
    void onRoomLeave(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Player>& who);

    void onEffectAdd(MudObject* parent, EffectInfo* ei);
    void onEffectRemove(MudObject* parent, EffectInfo* ei);

    void onChannelText(const std::shared_ptr<Player>& recipient, std::string_view channel, std::string_view talker, std::string_view text);
    std::string commTypeChannel(int type);
    nlohmann::json commChannelList(const std::shared_ptr<Player>& player);

    void markRoomDirty(const std::shared_ptr<Player>& player);
}

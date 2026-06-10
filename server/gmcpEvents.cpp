/*
 * gmcpEvents.cpp
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

#include <nlohmann/json.hpp>

#include "gmcpEvents.hpp"
#include "communication.hpp"           // for channelList, COM_* types
#include "effects.hpp"                 // for EffectInfo, Effect
#include "msdp.hpp"                    // for ReportedMsdpVariable
#include "mudObjects/mudObject.hpp"    // for MudObject
#include "mudObjects/objects.hpp"      // for Object
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom
#include "proto.hpp"                   // for listObjectSee, roomPlayerVisible
#include "socket.hpp"                  // for Socket

namespace {
    struct LocalChannel { int type; const char* name; bool usesLanguage; };
    const LocalChannel localChannels[] = {
        {COM_SAY,     "say",     true},
        {COM_TELL,    "tell",    true},
        {COM_REPLY,   "reply",   true},
        {COM_WHISPER, "whisper", true},
        {COM_SIGN,    "sign",    true},
        {COM_RECITE,  "recite",  true},
        {COM_YELL,    "yell",    true},
        {COM_GT,      "group",   true},
        {COM_EMOTE,   "emote",   false},
    };

    void emit(const std::shared_ptr<Player>& player, const std::string& pkg, const nlohmann::json& body) {
        if(!player) return;
        auto sock = player->getSock();
        if(!sock || !sock->gmcpEnabled() || !sock->gmcpSupports(pkg)) return;
        sock->gmcpSend(pkg, body);
    }
}

void gmcp::onInventoryAdd(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>& obj) {
    if(!player || !obj) return;
    emit(player, "Char.Items.Add", {{"location", "inv"}, {"item", {{"id", obj->getId()}, {"name", obj->getName()}}}});
}

void gmcp::onInventoryRemove(const std::shared_ptr<Player>& player, const std::shared_ptr<Object>& obj) {
    if(!player || !obj) return;
    emit(player, "Char.Items.Remove", {{"location", "inv"}, {"item", {{"id", obj->getId()}}}});
}

void gmcp::onRoomItemAdd(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Object>& obj) {
    if(!room || !obj) return;
    nlohmann::json body = {{"location", "room"}, {"item", {{"id", obj->getId()}, {"name", obj->getName()}}}};
    for(const auto& wp : room->players)
        if(auto p = wp.lock())
            if(listObjectSee(p, obj, false))
                emit(p, "Char.Items.Add", body);
}

void gmcp::onRoomItemRemove(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Object>& obj) {
    if(!room || !obj) return;
    nlohmann::json body = {{"location", "room"}, {"item", {{"id", obj->getId()}}}};
    for(const auto& wp : room->players)
        if(auto p = wp.lock())
            if(listObjectSee(p, obj, false))
                emit(p, "Char.Items.Remove", body);
}

void gmcp::onRoomEnter(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Player>& who) {
    if(!room || !who) return;
    nlohmann::json body = {{"name", who->getName()}, {"fullname", who->fullName()}};
    for(const auto& wp : room->players) {
        auto p = wp.lock();
        if(p && p != who && roomPlayerVisible(p, who))
            emit(p, "Room.AddPlayer", body);
    }
    markRoomDirty(who);
}

void gmcp::onRoomLeave(const std::shared_ptr<BaseRoom>& room, const std::shared_ptr<Player>& who) {
    if(!room || !who) return;
    nlohmann::json body = {{"name", who->getName()}};
    for(const auto& wp : room->players) {
        auto p = wp.lock();
        if(p && p != who && roomPlayerVisible(p, who))
            emit(p, "Room.RemovePlayer", body);
    }
}

void gmcp::onEffectAdd(MudObject* parent, EffectInfo* ei) {
    if(!parent || !ei || !ei->getEffect()) return;
    auto player = parent->getAsPlayer();
    if(!player) return;
    bool positive = ei->getEffect()->getType() == "Positive";
    emit(player, positive ? "Char.Defences.Add" : "Char.Afflictions.Add",
         {{"name", ei->getDisplayName()}, {"duration", ei->getDuration()}, {"strength", ei->getStrength()}});
}

void gmcp::onEffectRemove(MudObject* parent, EffectInfo* ei) {
    if(!parent || !ei || !ei->getEffect()) return;
    auto player = parent->getAsPlayer();
    if(!player) return;
    bool positive = ei->getEffect()->getType() == "Positive";
    emit(player, positive ? "Char.Defences.Remove" : "Char.Afflictions.Remove",
         {{"name", ei->getDisplayName()}});
}

void gmcp::onChannelText(const std::shared_ptr<Player>& recipient, std::string_view channel, std::string_view talker, std::string_view text) {
    if(!recipient) return;
    emit(recipient, "Comm.Channel.Text", {{"channel", std::string(channel)}, {"talker", std::string(talker)}, {"text", std::string(text)}});
}

std::string gmcp::commTypeChannel(int type) {
    for(const auto& c : localChannels)
        if(c.type == type)
            return c.name;
    return "say";
}

nlohmann::json gmcp::commChannelList(const std::shared_ptr<Player>& player) {
    nlohmann::json arr = nlohmann::json::array();
    if(!player) return arr;
    for(int i = 0; channelList[i].channelName != nullptr; i++) {
        const channelInfo& c = channelList[i];
        if(c.canSee && !c.canSee(player)) continue;
        if(c.canUse && !c.canUse(player)) continue;
        arr.push_back({{"name", c.channelName}, {"language", c.useLanguage}});
    }
    // Advertise the targeted/local channels too so every Comm.Channel.Text identifier appears here.
    for(const auto& c : localChannels)
        arr.push_back({{"name", c.name}, {"language", c.usesLanguage}});
    return arr;
}

void gmcp::markRoomDirty(const std::shared_ptr<Player>& player) {
    if(!player) return;
    auto sock = player->getSock();
    if(!sock) return;
    if(auto* rv = sock->getReportedMsdpVariable("ROOM"))
        rv->setDirty(true);
}

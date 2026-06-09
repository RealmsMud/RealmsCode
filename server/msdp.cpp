/*
 * mdsp.cpp
 *   Stuff to deal with MDSP
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

#include <arpa/telnet.h>               // for IAC, SB, SE
#include <cmath>                       // for floor, round
#include <functional>                  // for function, operator==
#include <list>                        // for operator==, list, _List_const_...
#include <map>                         // for operator==, map, _Rb_tree_iter...
#include <set>                         // for set (dirty GMCP packages)
#include <sstream>                     // for operator<<, basic_ostream, ost...
#include <string>                      // for string, operator<<, char_traits
#include <string_view>                 // for string_view, operator<<, basic...
#include <utility>                     // for tuple_element<>::type, pair
#include <vector>                      // for vector

#include "area.hpp"                    // for MapMarker
#include "catRef.hpp"                  // for CatRef
#include "config.hpp"                  // for Config, gConfig, MsdpVariableMap
#include "flags.hpp"                   // for P_NO_SHOW_STATS, P_DM_INVIS
#include "gmcp.hpp"                    // for the GMCP conversion layer
#include "group.hpp"                   // for Group, CreatureList, GROUP_INV...
#include "location.hpp"                // for Location
#include "login.hpp"                   // for CON_DISCONNECTING, CON_PLAYING
#include "raceData.hpp"                // for RaceData
#include "msdp.hpp"                    // for ReportedMsdpVariable, MsdpVari...
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/creatures.hpp"    // for Creature
#include "effects.hpp"                 // for EffectInfo, Effect
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/objects.hpp"      // for Object
#include "mudObjects/players.hpp"      // for Player
#include "skills.hpp"                  // for Skill
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "server.hpp"                  // for Server, SocketList
#include "socket.hpp"                  // for Socket, MSDP_VAL, MSDP_VAR
#include "stats.hpp"                   // for Stat
#include "timer.hpp"                   // for Timer

// Opt-in: build with -DMSDP_DEBUG to trace MSDP/GMCP traffic. Off by default --
// these clogs sit on the per-tick report path.

void Server::processReporting() {
    for(const auto& sock : sockets) {
        if(sock->getState() == CON_DISCONNECTING)
            continue;

        bool useMsdp = sock->msdpEnabled();
        bool useGmcp = sock->gmcpEnabled();
        if((!useMsdp && !useGmcp) || sock->msdpReporting.empty())
            continue;

        nlohmann::json gmcpMsdpBatch;        // channel 1: MSDP-over-GMCP
        std::set<std::string> dirtyPackages; // channel 2: standard packages

        for(auto& [vName, var] : sock->msdpReporting) {
            if(var.getRequiresPlayer() && (!sock->getPlayer() || sock->getState() != CON_PLAYING)) continue;
            // Recompute on the timer or when an event forced the var dirty
            bool due = var.checkTimer();
            if(due || var.isDirty())
                var.update();
            if(!var.isDirty()) continue;

            if(useMsdp)
                var.send(*sock);

            if(useGmcp) {
                if(sock->gmcpMsdpVars.find(var.getName()) != sock->gmcpMsdpVars.end())
                    gmcpMsdpBatch[var.getName()] = gmcp::msdpValueToJson(var.getValue());
                std::string pkg = gmcp::packageForVar(var.getName());
                if(!pkg.empty() && sock->gmcpSupports(pkg))
                    dirtyPackages.insert(pkg);
            }

            var.setDirty(false);
        }

        if(useGmcp) {
            if(gmcpMsdpBatch.is_object() && !gmcpMsdpBatch.empty())
                sock->gmcpSend("MSDP", gmcpMsdpBatch);
            for(const auto& pkg : dirtyPackages)
                sock->gmcpSendPackage(pkg);
        }
    }
}

bool Socket::processMsdpVarVal(const std::string &variable, const std::string &value) {
#ifdef MSDP_DEBUG
    std::clog << "Found Var: '" << variable << "' Val: '" << value << "'" << std::endl;
#endif
    if (variable == "LIST") {
        return (msdpList(value));
    }
    else if (variable == "REPORT") {
#ifdef MSDP_DEBUG
        std::clog << "msdpReport(" << value << ")" << std::endl;
#endif
        return (msdpReport(value) != nullptr);
    }
    else if (variable == "UNREPORT") {
        return (msdpUnReport(value));
    }
    else if (variable == "SEND") {
        return (msdpSend(value));
    }
    else {
        // See if they've sent us a configurable variable, if so set it
        auto it = gConfig->msdpVariables.find(variable);
        if(it != gConfig->msdpVariables.end()) {
            MsdpVariable* msdpVar = &it->second;

            if(msdpVar->isConfigurable()) {
                ReportedMsdpVariable* reportedVar = nullptr;
                reportedVar = getReportedMsdpVariable(variable);
                if(reportedVar == nullptr) {
                    // If we're not reporting, start reporting it
                    reportedVar = msdpReport(variable);
                }

                // Should never happen since we just added it
                if(reportedVar == nullptr)
                    return(false);

                // If it's a write-once variable, we can only set it if the value is currently unknown
                if(msdpVar->isWriteOnce() && reportedVar->getValue() != "unknown")
                    return(false);

                reportedVar->setValue(value);
#ifdef MSDP_DEBUG
                std::clog << "processMsdpVarVal: Set configurable variable '" << variable << "' to '" << value << "'" << std::endl;
#endif
                return(true);
            }
        }
#ifdef MSDP_DEBUG
        std::clog << "processMsdpVarVal: Unknown variable '" << variable << "'" << std::endl;
#endif
    }
    return (true);
}


const std::vector<std::string> MsdpCommandList = { "LIST", "REPORT", "RESET", "SEND", "UNREPORT" };
const std::vector<std::string> MsdpLists = { "COMMANDS", "LISTS", "CONFIGURABLE_VARIABLES", "REPORTABLE_VARIABLES", "REPORTED_VARIABLES", "SENDABLE_VARIABLES" };

std::vector<std::string> Socket::msdpListValues(const std::string &which, std::string &label) {
    std::vector<std::string> out;
    label = which;

    if (which == "COMMANDS") {
        out = MsdpCommandList;
    }
    else if (which == "LISTS") {
        out = MsdpLists;
    }
    else if (which == "SENDABLE_VARIABLES" || which.empty()) {
        label = "SENDABLE_VARIABLES";
        for (auto& [vName, var] : gConfig->msdpVariables) out.push_back(var.getName());
    }
    else if (which == "REPORTABLE_VARIABLES") {
        for (auto& [vName, var] : gConfig->msdpVariables) if (var.isReportable()) out.push_back(var.getName());
    }
    else if (which == "CONFIGURABLE_VARIABLES") {
        for (auto& [vName, var] : gConfig->msdpVariables) if (var.isConfigurable()) out.push_back(var.getName());
    }
    else if (which == "REPORTED_VARIABLES") {
        for (auto& [vName, var] : msdpReporting) out.push_back(var.getName());
    }
    else {
        label.clear();
    }
    return out;
}

bool Socket::msdpList(const std::string &value) {
    std::string label;
    std::vector<std::string> values = msdpListValues(value, label);
    if (label.empty())
        return (false);
    msdpSendList(label, values);
    return (true);
}

ReportedMsdpVariable* Socket::getReportedMsdpVariable(const std::string &value) {
    auto it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (nullptr);
    else
        return &(it->second);
}

std::string Socket::getMsdpReporting() {
    std::ostringstream ostr;
    for(auto &[vName, var] : msdpReporting) {
        ostr << var.getName() << " ";
    }
    return ostr.str();
}

std::string Socket::getGmcpPackages() {
    std::string out;
    for(const auto& pkg : gmcpStdPackages) {
        out += pkg;
        out += ' ';
    }
    return out;
}

nlohmann::json Socket::gmcpRoomInfo() {
    auto player = getPlayer();
    if(!player)
        return nlohmann::json(nullptr);
    auto room = player->getRoomParent();
    if(!room)
        return nlohmann::json(nullptr);

    nlohmann::json j = nlohmann::json::object();
    if(auto uRoom = room->getAsUniqueRoom()) {
        j["num"] = uRoom->info.id;
        j["area"] = uRoom->info.area;
    } else if(auto aRoom = room->getAsAreaRoom()) {
        const MapMarker& mm = aRoom->mapmarker;
        j["area"] = mm.getArea();
        j["coords"] = {{"x", mm.getX()}, {"y", mm.getY()}, {"z", mm.getZ()}};
    }
    j["name"] = room->getName();

    nlohmann::json exits = nlohmann::json::object();
    for(const auto& exit : room->exits) {
        // Hide secret/concealed/invisible exits the viewer can't perceive.
        if(!player->showExit(exit))
            continue;
        // Unique-room targets carry a stable room number; area-room targets do not.
        exits[exit->getName()] = exit->target.mapmarker.getArea() ? 0 : exit->target.room.id;
    }
    j["exits"] = exits;
    return j;
}

nlohmann::json Socket::gmcpCharGroup() {
    std::shared_ptr<Creature> viewer = getPlayer();
    if(!viewer || !getPlayer()->getGroup())
        return nlohmann::json(nullptr);
    auto group = getPlayer()->getGroup();

    nlohmann::json j = nlohmann::json::object();
    j["name"] = group->getName();
    j["type"] = group->getGroupTypeStr();

    nlohmann::json members = nlohmann::json::array();
    for(const auto& weakTarget : group->members) {
        const auto& target = weakTarget.lock();
        if(!target)
            continue;
        if(!viewer->isStaff() && (target->pFlagIsSet(P_DM_INVIS) || (target->isEffected("incognito") && !viewer->inSameRoom(target))))
            continue;
        if(target->getGroupStatus() == GROUP_INVITED)
            continue;

        auto master = target->getMaster();
        bool isPet = target->isPet();
        if(isPet && !master)  // pet outlived its master; nothing coherent to report
            continue;
        bool showStats = (viewer->isCt() ||
            (isPet && !master->flagIsSet(P_NO_SHOW_STATS)) ||
            (!isPet && !target->pFlagIsSet(P_NO_SHOW_STATS)) ||
            (isPet && master == viewer) ||
            (!isPet && target == viewer));

        nlohmann::json m = nlohmann::json::object();
        m["name"]  = isPet ? (master->getName() + "'s " + target->getName()) : target->getName();
        m["hp"]    = showStats ? target->hp.getCur() : -1;
        m["maxhp"] = showStats ? target->hp.getMax() : -1;
        m["mp"]    = showStats ? target->mp.getCur() : -1;
        m["maxmp"] = showStats ? target->mp.getMax() : -1;

        nlohmann::json effects = nlohmann::json::array();
        if(!isPet) {
            if(target->isEffected("blindness"))    effects.push_back("Blind");
            if(target->isEffected("drunkenness"))  effects.push_back("Drunk");
            if(target->isEffected("confusion"))    effects.push_back("Confused");
            if(target->isDiseased())               effects.push_back("Diseased");
            if(target->isEffected("petrification"))effects.push_back("Petrified");
            if(target->isPoisoned())               effects.push_back("Poisoned");
            if(target->isEffected("silence"))      effects.push_back("Silenced");
            if(target->flagIsSet(P_SLEEPING))      effects.push_back("Sleeping");
            else if(target->flagIsSet(P_UNCONSCIOUS)) effects.push_back("Unconscious");
            if(target->isEffected("wounded"))      effects.push_back("Wounded");
        }
        m["effects"] = effects;
        m["room"] = (showStats && target->getRoomParent()) ? target->getRoomParent()->getName() : "";

        members.push_back(m);
    }
    j["members"] = members;
    return j;
}

nlohmann::json Socket::gmcpRoomPlayers() {
    nlohmann::json arr = nlohmann::json::array();
    auto player = getPlayer();
    if(!player || !player->getRoomParent())
        return arr;
    for(const auto& p : player->getRoomParent()->getVisiblePlayers(player))
        arr.push_back({{"name", p->getName()}, {"fullname", p->fullName()}});
    return arr;
}

nlohmann::json Socket::gmcpSkillGroups() {
    nlohmann::json arr = nlohmann::json::array();
    auto player = getPlayer();
    if(!player)
        return arr;
    std::set<std::string> groups;
    for(const auto& [name, skill] : player->skills)
        if(skill && !skill->getGroup().empty())
            groups.insert(skill->getGroup());
    for(const auto& g : groups)
        arr.push_back(g);
    return arr;
}

nlohmann::json Socket::gmcpSkillList(const std::string& group) {
    nlohmann::json j = nlohmann::json::object();
    j["group"] = group;
    nlohmann::json list = nlohmann::json::array();
    auto player = getPlayer();
    if(player)
        for(const auto& [name, skill] : player->skills) {
            if(!skill)
                continue;
            if(!group.empty() && skill->getGroup() != group)
                continue;
            list.push_back({{"name", skill->getDisplayName()}, {"rank", skill->getGained()}});
        }
    j["list"] = list;
    return j;
}

nlohmann::json Socket::gmcpEffectList(bool defences) {
    nlohmann::json arr = nlohmann::json::array();
    auto player = getPlayer();
    if(!player)
        return arr;
    for(EffectInfo* ei : player->effects.effectList) {
        if(!ei || !ei->getEffect())
            continue;
        if((ei->getEffect()->getType() == "Positive") != defences)
            continue;
        arr.push_back({
            {"name",     ei->getDisplayName()},
            {"duration", ei->getDuration()},
            {"strength", ei->getStrength()},
        });
    }
    return arr;
}

nlohmann::json Socket::gmcpItemsList(const std::string& location) {
    nlohmann::json j = nlohmann::json::object();
    j["location"] = location;
    nlohmann::json items = nlohmann::json::array();
    auto player = getPlayer();
    if(player) {
        if(location == "inv") {
            // Own inventory: no visibility gate.
            for(const auto& obj : player->objects)
                if(obj)
                    items.push_back({{"id", obj->getId()}, {"name", obj->getName()}});
        } else if(location == "room" && player->getRoomParent()) {
            for(const auto& obj : player->getRoomParent()->getVisibleObjects(player))
                items.push_back({{"id", obj->getId()}, {"name", obj->getName()}});
        }
    }
    j["items"] = items;
    return j;
}

ReportedMsdpVariable* Socket::msdpReport(const std::string &value) {
    MsdpVariable* msdpVar = gConfig->getMsdpVariable(value);

    if(!msdpVar || msdpVar->getName() != value) {
#ifdef MSDP_DEBUG
        std::clog << "MsdpHandleReport: Unknown VAR '" << value << "'" << std::endl;
#endif
        return nullptr;
    }

    if (!msdpVar->isReportable()) {
#ifdef MSDP_DEBUG
        std::clog << "MsdpHandleReport: Un-Reportable VAR '" << value << "'" << std::endl;
#endif
        return nullptr;
    }

    ReportedMsdpVariable* reported = getReportedMsdpVariable(value);
    if (reported != nullptr) {
#ifdef MSDP_DEBUG
        std::clog << "MsdpHandleReport: Already Reporting '" << value << "'" << std::endl;
#endif
        return reported;
    }

    msdpReporting.emplace(msdpVar->getName(), ReportedMsdpVariable(msdpVar, shared_from_this()));
#ifdef MSDP_DEBUG
    std::clog << "MsdpHandleReport: Now Reporting '" << msdpVar->getName() << "'" << std::endl;
#endif
    return &msdpReporting.at(msdpVar->getName());
}

bool Socket::msdpReset(std::string& value) {
    if (value == "REPORTABLE_VARIABLES" || value == "SENDABLE_VARIABLES") {
        // For now just clear all reported variables
        msdpClearReporting();
    }
    return (false);
}

void Socket::msdpClearReporting() {
    msdpReporting.clear();
}

bool Socket::msdpSend(const std::string &variable) {
    MsdpVariable *msdpVar = gConfig->getMsdpVariable(variable);
    if(msdpVar == nullptr) {
#ifdef MSDP_DEBUG
        std::clog << "Unknown variable to send: '" << variable << "'" << std::endl;
#endif
        return (false);
    }
    return (msdpVar->send(*this));
}


bool Socket::msdpUnReport(const std::string &value) {
    auto it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (false);
    else {
#ifdef MSDP_DEBUG
        std::clog << "MsdpHandleUnReport: No longer reporting '" << value << "'" << std::endl;
#endif
        msdpReporting.erase(it);
        return (true);
    }
}

void Socket::msdpSendList(std::string_view variable, const std::vector<std::string>& values) {
    if (!msdpEnabled())
        return;

    std::string body;
    body.push_back((char) MSDP_VAR);
    body.append(variable);
    body.push_back((char) MSDP_VAL);
    body.push_back((char) MSDP_ARRAY_OPEN);
    for (const auto& value : values) {
        body.push_back((char) MSDP_VAL);
        body.append(telnet::escapeIAC(value));
    }
    body.push_back((char) MSDP_ARRAY_CLOSE);
    // Values are pre-escaped above; do not re-escape the assembled MSDP body.
    std::string toSend = telnet::subnegotiate(TELOPT_MSDP, body, false);

    writeRaw(toSend);
}

void debugMsdp(std::string_view str) {
    bool iac = false;

    std::ostringstream oStr;
    for ( const auto& ch : str) {
        switch(ch) {
            case (unsigned char) IAC:
                oStr << " IAC ";
                iac = true;
                break;
            case (unsigned char) SB:
                oStr << " SB ";
                break;
            case (unsigned char) SE:
                oStr << " SE ";
                iac = false;
                break;
            case (unsigned char) TELOPT_MSDP:
                if(iac) {
                    oStr << " TELOPT_MSDP ";
                    iac = false;
                } else {
                    oStr << ch;
                }
                break;
            case (unsigned char) MSDP_VAR:
                oStr << " MSDP_VAR ";
                break;
            case (unsigned char) MSDP_VAL:
                oStr << " MSDP_VAL ";
                break;
            case (unsigned char) MSDP_TABLE_OPEN:
                oStr << " MSDP_TABLE_OPEN ";
                break;
            case (unsigned char) MSDP_TABLE_CLOSE:
                oStr << " MSDP_TABLE_CLOSE ";
                break;
            case (unsigned char) MSDP_ARRAY_OPEN:
                oStr << " MSDP_ARRAY_OPEN ";
                break;
            case (unsigned char) MSDP_ARRAY_CLOSE:
                oStr << " MSDP_ARRAY_CLOSE ";
                break;
            default:
                oStr << ch;
                break;

        }
    }
    std::clog << oStr.str() << std::endl;
}

bool Socket::msdpSendPair(std::string_view variable, std::string_view value) {
    if (variable.empty() || value.empty())
        return false;

    if (!this->msdpEnabled())
        return true;

    std::string body;
    body.reserve(variable.size() + value.size() + 2);
    body.push_back((char) MSDP_VAR);
    body.append(variable);
    body.push_back((char) MSDP_VAL);
    body.append(telnet::escapeIAC(value));
    // Value is pre-escaped above; do not re-escape the assembled MSDP body.
    std::string toSend = telnet::subnegotiate(TELOPT_MSDP, body, false);

#ifdef MSDP_DEBUG
    debugMsdp(toSend);
#endif

    writeRaw(toSend);
    return true;
}

MsdpVariable* Config::getMsdpVariable(const std::string &name) {
    auto it = msdpVariables.find(name);
    if(it == msdpVariables.end())
        return(nullptr);
    else
        return &(it->second);
}

ReportedMsdpVariable::ReportedMsdpVariable(const MsdpVariable* mv, const std::shared_ptr<Socket>& sock) {
    name = mv->getName();
    parentSock = sock;
    configurable = mv->isConfigurable();
    writeOnce = mv->isWriteOnce();
    valueFn = mv->valueFn;
    updateable = mv->updateable;
    updateInterval = mv->getUpdateInterval();
    reportable = mv->isReportable();
    timer.setDelay(updateInterval);

    dirty = true;
    value = "unknown";
}

const std::string & MsdpVariable::getName() const {
    return(name);
}

bool MsdpVariable::isConfigurable() const {
    return(configurable);
}

bool MsdpVariable::isReportable() const {
    return(reportable);
}

bool MsdpVariable::isWriteOnce() const {
    return(writeOnce);
}

bool MsdpVariable::getRequiresPlayer() const {
    return(requiresPlayer);
}

int MsdpVariable::getUpdateInterval() const {
    return(updateInterval);
}

bool MsdpVariable::hasValueFn() const {
    return(valueFn != nullptr);
}

bool MsdpVariable::isUpdatable() const {
    return(updateable);
}

std::string MsdpVariable::currentValue(Socket &sock) const {
    if (!hasValueFn()) {
        if (!isConfigurable()) return std::string();
        ReportedMsdpVariable* reported = sock.getReportedMsdpVariable(name);
        return reported != nullptr ? reported->getValue() : std::string();
    }
    if (requiresPlayer && !sock.hasPlayer()) return std::string();
    return valueFn(sock, sock.getPlayer());
}

bool MsdpVariable::send(Socket &sock) const {
    if (!hasValueFn()) {
        // If there's no send function, and it's not configurable, there's nothing we can do
        if(!isConfigurable()) return false;
    } else if (requiresPlayer && !sock.hasPlayer()) {
        return false;
    }

    sock.msdpSendPair(getName(), currentValue(sock));
    return true;
}

const std::string& ReportedMsdpVariable::getValue() const {
    return(value);
}

bool ReportedMsdpVariable::checkTimer() {
    if(timer.hasExpired()) {
        timer.update(getUpdateInterval());
        return(true);
    } else {
        return(false);
    }
}

void ReportedMsdpVariable::setValue(std::string_view newValue) {
    if(value != newValue) {
        value = newValue;
        dirty = true;
    }
}

void ReportedMsdpVariable::setValue(int newValue) {
    return(setValue(std::to_string(newValue)));
}

void ReportedMsdpVariable::setValue(long newValue) {
    return(setValue(std::to_string(newValue)));
}

bool ReportedMsdpVariable::isDirty() const {
    return(dirty);
}

void ReportedMsdpVariable::update() {
    if(!isUpdatable()) return;
    auto sock = parentSock.lock();
    if(!sock) return;

    setValue(MsdpVariable::valueFn(*sock, sock->getPlayer()));
}

void ReportedMsdpVariable::setDirty(bool pDirty) {
    dirty = pDirty;
}

std::string BaseRoom::getExitsMsdp(const std::shared_ptr<const Player>& viewer) const {
    std::ostringstream oStr;

    if (!exits.empty()) {
        oStr << (unsigned char) MSDP_VAR << "EXITS"
             << (unsigned char) MSDP_VAL << (unsigned char) MSDP_TABLE_OPEN;

        for (const auto& exit : exits ) {
            // Hide secret/concealed/invisible exits the viewer can't perceive.
            if(viewer && !viewer->showExit(exit))
                continue;
            oStr << (unsigned char) MSDP_VAR << exit->getName()
                 << (unsigned char) MSDP_VAL << (unsigned char) MSDP_TABLE_OPEN;

            if(exit->target.mapmarker.getArea()) {
                oStr << (unsigned char) MSDP_VAR << "A"
                     << (unsigned char) MSDP_VAL << exit->target.mapmarker.getArea()
                     << (unsigned char) MSDP_VAR << "X"
                     << (unsigned char) MSDP_VAL << exit->target.mapmarker.getX()
                     << (unsigned char) MSDP_VAR << "Y"
                     << (unsigned char) MSDP_VAL << exit->target.mapmarker.getY()
                     << (unsigned char) MSDP_VAR << "Z"
                     << (unsigned char) MSDP_VAL << exit->target.mapmarker.getZ();
            }
            else {
                oStr << (unsigned char) MSDP_VAR << "AREA"
                     << (unsigned char) MSDP_VAL << exit->target.room.area
                     << (unsigned char) MSDP_VAR << "NUM"
                     << (unsigned char) MSDP_VAL << exit->target.room.id;
            }

            oStr << (unsigned char) MSDP_TABLE_CLOSE;

        }

        oStr << (unsigned char) MSDP_TABLE_CLOSE;

    }
    return oStr.str();
}

std::string UniqueRoom::getMsdp(const std::shared_ptr<const Player>& viewer, bool showExits) const {
    std::ostringstream oStr;

    oStr << (unsigned char) MSDP_TABLE_OPEN

         << (unsigned char) MSDP_VAR << "AREA"
         << (unsigned char) MSDP_VAL << info.area

         << (unsigned char) MSDP_VAR << "NUM"
         << (unsigned char) MSDP_VAL << info.id

         << (unsigned char) MSDP_VAR << "NAME"
         << (unsigned char) MSDP_VAL << getName();

    if (showExits)
        oStr << getExitsMsdp(viewer);

    oStr << (unsigned char) MSDP_TABLE_CLOSE;

    return oStr.str();
}


std::string AreaRoom::getMsdp(const std::shared_ptr<const Player>& viewer, bool showExits) const {
    std::ostringstream oStr;

    oStr << (unsigned char) MSDP_TABLE_OPEN

         << (unsigned char) MSDP_VAR << "AREA"
         << (unsigned char) MSDP_VAL << mapmarker.getArea()

         << (unsigned char) MSDP_VAR << "NAME"
         << (unsigned char) MSDP_VAL << getName()

         << (unsigned char) MSDP_VAR << "COORDS"
         << (unsigned char) MSDP_VAL << (unsigned char) MSDP_TABLE_OPEN

             << (unsigned char) MSDP_VAR << "X"
             << (unsigned char) MSDP_VAL << mapmarker.getX()
             << (unsigned char) MSDP_VAR << "Y"
             << (unsigned char) MSDP_VAL << mapmarker.getY()
             << (unsigned char) MSDP_VAR << "Z"
             << (unsigned char) MSDP_VAL << mapmarker.getZ()

          << (unsigned char) MSDP_TABLE_CLOSE;

    if (showExits)
        oStr << getExitsMsdp(viewer);

    oStr << (unsigned char) MSDP_TABLE_CLOSE;
    return oStr.str();
}

std::string Group::getMsdp(const std::shared_ptr<Creature>& viewer) const {
    int i = 0;
    std::ostringstream oStr;

    oStr << (unsigned char) MSDP_TABLE_OPEN // Group

         << (unsigned char) MSDP_VAR << "NAME"
         << (unsigned char) MSDP_VAL << getName()

         << (unsigned char) MSDP_VAR << "TYPE"
         << (unsigned char) MSDP_VAL << getGroupTypeStr()

         << (unsigned char) MSDP_VAR << "XPSPLIT"
         << (unsigned char) MSDP_VAL << (flagIsSet(GROUP_SPLIT_EXPERIENCE) ? "on" : "off")

         << (unsigned char) MSDP_VAR << "GOLDSPLIT"
         << (unsigned char) MSDP_VAL << (flagIsSet(GROUP_SPLIT_GOLD) ? "on" : "off")

         << (unsigned char) MSDP_VAR << "MEMBERS"
         << (unsigned char) MSDP_VAL << (unsigned char) MSDP_TABLE_OPEN; // Members


        for(const auto& weakTarget : members) {
            const auto& target = weakTarget.lock();

            if(!viewer->isStaff() && (target->pFlagIsSet(P_DM_INVIS) || (target->isEffected("incognito") && !viewer->inSameRoom(target))))
                continue;

            if(target->getGroupStatus() == GROUP_INVITED)
                continue;

            bool isPet = target->isPet();

            oStr << (unsigned char) MSDP_VAR << ++i
                 << (unsigned char) MSDP_VAL << (unsigned char) MSDP_TABLE_OPEN;  // Member

            oStr << (unsigned char) MSDP_VAR << "NAME"
                 << (unsigned char) MSDP_VAL;

            if(isPet)
                oStr << target->getMaster()->getName() << "'s " << target->getName();
            else
                oStr << target->getName();

            bool showStats = ( viewer->isCt() ||
                (isPet && !target->getMaster()->flagIsSet(P_NO_SHOW_STATS)) ||
                (!isPet && !target->pFlagIsSet(P_NO_SHOW_STATS)) ||
                (isPet && target->getMaster() == viewer) ||
                (!isPet && target == viewer));

            oStr << (unsigned char) MSDP_VAR << "HEALTH"
                 << (unsigned char) MSDP_VAL << (showStats ? target->hp.getCur() : -1)
                 << (unsigned char) MSDP_VAR << "HEALTH_MAX"
                 << (unsigned char) MSDP_VAL << (showStats ? target->hp.getMax() : -1)
                 << (unsigned char) MSDP_VAR << "MANA"
                 << (unsigned char) MSDP_VAL << (showStats ? target->mp.getCur() : -1)
                 << (unsigned char) MSDP_VAR << "MANA_MAX"
                 << (unsigned char) MSDP_VAL << (showStats ? target->mp.getMax() : -1);

            oStr << (unsigned char) MSDP_VAR << "EFFECTS"
                 << (unsigned char) MSDP_VAL << (unsigned char) MSDP_ARRAY_OPEN;

                if(!isPet) {
                    if (target->isEffected("blindness"))
                        oStr << MSDP_VAL << "Blind";
                    if (target->isEffected("drunkenness"))
                        oStr << MSDP_VAL << "Drunk";
                    if (target->isEffected("confusion"))
                        oStr << MSDP_VAL << "Confused";
                    if (target->isDiseased())
                        oStr << MSDP_VAL << "Diseased";
                    if (target->isEffected("petrification"))
                        oStr << MSDP_VAL << "Petrified";
                    if (target->isPoisoned())
                        oStr << MSDP_VAL << "Poisoned";
                    if (target->isEffected("silence"))
                        oStr << MSDP_VAL << "Silenced";
                    if (target->flagIsSet(P_SLEEPING))
                        oStr << MSDP_VAL << "Sleeping";
                    else if (target->flagIsSet(P_UNCONSCIOUS))
                        oStr << MSDP_VAL << "Unconscious";
                    if (target->isEffected("wounded"))
                        oStr << MSDP_VAL << "Wounded";
                }

                oStr << (unsigned char) MSDP_ARRAY_CLOSE;

            oStr  << (unsigned char) MSDP_VAR << "ROOM"
                  << (unsigned char) MSDP_VAL << (showStats ? target->getRoomParent()->getMsdp(nullptr, false) : "");

            oStr << (unsigned char) MSDP_TABLE_CLOSE;  // Member
        }

        oStr << (unsigned char) MSDP_TABLE_CLOSE;  // Members

    oStr << (unsigned char) MSDP_TABLE_CLOSE; // Group

    return (oStr.str());

}

namespace msdp {
    const std::string UNKNOWN_STR = "unknown";
    const std::string NONE_STR = "none";

    std::string getServerId(Socket &sock, const std::shared_ptr<Player>& player) {
        return (gConfig->getMudNameAndVersion());
    }

    std::string getServerTime(Socket &sock, const std::shared_ptr<Player>& player) {
        return (Server::getServerTime());
    }

    const std::string &getCharacterName(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? player->getName() : UNKNOWN_STR);
    }

    std::string getHealth(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->hp.getCur()) : UNKNOWN_STR);
    }
    std::string getHealthMax(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->hp.getMax()) : UNKNOWN_STR);
    }
    std::string getMana(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->mp.getCur()) : UNKNOWN_STR);
    }
    std::string getManaMax(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->mp.getMax()) : UNKNOWN_STR);
    }
    std::string getExperience(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->getExperience()) : UNKNOWN_STR);
    }
    std::string getExperienceMax(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? player->expNeededDisplay() : UNKNOWN_STR);
    }
    std::string getExperienceTNL(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->expToLevel()) : UNKNOWN_STR);
    }
    std::string getExperienceTNLMax(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? player->expForLevel() : UNKNOWN_STR);
    }
    std::string getWimpy(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->getWimpy()) : UNKNOWN_STR);
    }
    std::string getMoney(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? player->getCoinDisplay() : UNKNOWN_STR);
    }
    std::string getBank(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? player->getBankDisplay() : UNKNOWN_STR);
    }
    std::string getArmor(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->getArmor()) : UNKNOWN_STR);
    }
    std::string getArmorAbsorb(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(floor(player->getDamageReduction(player)*100.0)) : UNKNOWN_STR);
    }
    std::string getGroup(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player && player->getGroup()) {
            return player->getGroup()->getMsdp(player);
        } else {
            return UNKNOWN_STR;
        }
    }
    const std::string& getTarget(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player) {
            auto target = player->getTarget();
            return(target ? target->getName() : NONE_STR);
        } else {
            return UNKNOWN_STR;
        }
    }
    const std::string& getTargetID(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player) {
            auto target = player->getTarget();
            return(target ? target->getId() : NONE_STR);
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getTargetHealth(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player && player->getTarget()) {
            auto target = player->getTarget();
            return(std::to_string(round((target->hp.getCur()*10000.0) / (target->hp.getMax()*1.0))/100));
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getTargetHealthMax(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player && player->getTarget()) {
            return std::to_string(100);
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getTargetStrength(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player && player->getTarget()) {
            // Not implemented
            return UNKNOWN_STR;
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getRoom(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player && player->getRoomParent()) {
            return player->getRoomParent()->getMsdp(player);
        } else
            return UNKNOWN_STR;
    }
    std::string getLevel(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? std::to_string(player->getLevel()) : UNKNOWN_STR);
    }
    std::string getClassName(Socket &sock, const std::shared_ptr<Player>& player) {
        return (player ? player->getClassString() : UNKNOWN_STR);
    }
    std::string getRaceName(Socket &sock, const std::shared_ptr<Player>& player) {
        if(player) {
            const RaceData* race = gConfig->getRace(player->getRace());
            if(race) return race->getName();
        }
        return UNKNOWN_STR;
    }

};

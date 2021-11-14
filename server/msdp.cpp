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
#include <sstream>                     // for operator<<, basic_ostream, ost...
#include <string>                      // for string, operator<<, char_traits
#include <string_view>                 // for string_view, operator<<, basic...
#include <utility>                     // for tuple_element<>::type, pair
#include <vector>                      // for vector

#include "area.hpp"                    // for MapMarker
#include "catRef.hpp"                  // for CatRef
#include "config.hpp"                  // for Config, gConfig, MsdpVariableMap
#include "flags.hpp"                   // for P_NO_SHOW_STATS, P_DM_INVIS
#include "group.hpp"                   // for Group, CreatureList, GROUP_INV...
#include "location.hpp"                // for Location
#include "login.hpp"                   // for CON_DISCONNECTING, CON_PLAYING
#include "msdp.hpp"                    // for ReportedMsdpVariable, MsdpVari...
#include "mudObjects/areaRooms.hpp"    // for AreaRoom
#include "mudObjects/creatures.hpp"    // for Creature
#include "mudObjects/exits.hpp"        // for Exit
#include "mudObjects/players.hpp"      // for Player
#include "mudObjects/rooms.hpp"        // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"  // for UniqueRoom
#include "server.hpp"                  // for Server, SocketList
#include "socket.hpp"                  // for Socket, MSDP_VAL, MSDP_VAR
#include "stats.hpp"                   // for Stat
#include "timer.hpp"                   // for Timer

#define MSDP_DEBUG

void Server::processMsdp() {
    for(auto &sock : sockets) {
        if(sock.getState() == CON_DISCONNECTING)
            continue;

        if(sock.mccpEnabled()) {
            for(auto& [vName, var] : sock.msdpReporting) {
                if(var.getRequiresPlayer() && (!sock.getPlayer() || sock.getState() != CON_PLAYING)) continue;
                if(!var.checkTimer()) continue;

                var.update();
                if(!var.isDirty()) continue;

                var.send(sock);
                var.setDirty(false);
            }
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
        std::clog << "msdpReport(" << value << ")" << std::endl;
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

                // If it's a write once variable, we can only set it if the value is currently unknown
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

bool Socket::msdpList(const std::string &value) {
    if (value == "COMMANDS") {
        msdpSendList(value, MsdpCommandList);
        return (true);
    }
    else if (value == "LISTS") {
        msdpSendList(value, MsdpLists);
        return (true);
    }
    else if (value == "SENDABLE_VARIABLES") {
        std::vector<std::string> sendable;
        for (auto& [vName, var] : gConfig->msdpVariables) {
            sendable.push_back(var.getName());
        }
        msdpSendList(value, sendable);
        return (true);
    }
    else if (value == "REPORTABLE_VARIABLES") {
        std::vector<std::string> reportable;
        for (auto& [vName, var] : gConfig->msdpVariables) {
            if(var.isReportable()) reportable.push_back(var.getName());
        }
        msdpSendList(value, reportable);
        return (true);
    }
    else if (value == "CONFIGURABLE_VARIABLES") {
        std::vector<std::string> configurable;
        for (auto& [vName, var] : gConfig->msdpVariables) {
            if (var.isConfigurable()) configurable.push_back(var.getName());
        }
        msdpSendList(value, configurable);
        return (true);
    }
    else if (value == "REPORTED_VARIABLES") {
        std::vector<std::string> reported;
        for (auto& [vName, var] : msdpReporting) {
            reported.push_back(var.getName());
        }
        msdpSendList("REPORTED_VARIABLES", reported);
        return (true);
    }
    else if (value.empty()) {
        // If we just get a LIST command, send off a list of all variables
        std::vector<std::string> all;
        for (auto& [vName, var] : gConfig->msdpVariables) {
            all.push_back(var.getName());
        }
        msdpSendList("SENDABLE_VARIABLES", all);
        return (true);
    }

    return (false);
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

    msdpReporting.emplace(msdpVar->getName(), ReportedMsdpVariable(msdpVar, this));
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
    std::ostringstream oStr;

    if (msdpEnabled()) {
        oStr    << (unsigned char) IAC << (unsigned char) SB << (unsigned char) TELOPT_MSDP
                << (unsigned char) MSDP_VAR << variable << (unsigned char) MSDP_VAL
                << (unsigned char) MSDP_ARRAY_OPEN;

        for( auto& value : values ) {
            oStr << (unsigned char) MSDP_VAL << value;
        }

        oStr << (unsigned char) MSDP_ARRAY_CLOSE << (unsigned char) IAC << (unsigned char) SE;
    }

    write(oStr.str());

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

    std::ostringstream oStr;

    if (this->msdpEnabled()) {
        std::clog << "SendPair:MSDP" << std::endl;
        oStr
                << (unsigned char) IAC << (unsigned char) SB << (unsigned char) TELOPT_MSDP
                << (unsigned char) MSDP_VAR << variable
                << (unsigned char) MSDP_VAL << value
                << (unsigned char) IAC << (unsigned char) SE;
    }
    std::string toSend = oStr.str();

#ifdef MSDP_DEBUG
    debugMsdp(toSend);
#endif

    write(toSend);
    return true;
}

MsdpVariable* Config::getMsdpVariable(const std::string &name) {
    auto it = msdpVariables.find(name);
    if(it == msdpVariables.end())
        return(nullptr);
    else
        return &(it->second);
}

ReportedMsdpVariable::ReportedMsdpVariable(const MsdpVariable* mv, Socket* sock) {
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

bool MsdpVariable::send(Socket &sock) const {
    std::string value;

    if (!hasValueFn()) {
        // If there's no send function, and it's not configurable, there's nothing we can do
        if(!isConfigurable()) return false;

        ReportedMsdpVariable* reported = sock.getReportedMsdpVariable(name);
        if (reported != nullptr) {
            value = reported->getValue();
        }

    } else {
        if (requiresPlayer && !sock.hasPlayer()) return false;

        value = valueFn(sock, sock.getPlayer());
    }

    sock.msdpSendPair(getName(), value);
    return true;
}

std::string ReportedMsdpVariable::getValue() const {
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
    if(!parentSock) return;

    std::string oldValue = value;
    setValue(MsdpVariable::valueFn(*parentSock, parentSock->getPlayer()));
}

void ReportedMsdpVariable::setDirty(bool pDirty) {
    dirty = pDirty;
}

std::string BaseRoom::getExitsMsdp() const {
    std::ostringstream oStr;

    if (!exits.empty()) {
        oStr << (unsigned char) MSDP_VAR << "EXITS"
             << (unsigned char) MSDP_VAL << (unsigned char) MSDP_TABLE_OPEN;

        for (auto exit : exits ) {
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

std::string UniqueRoom::getMsdp(bool showExits) const {
    std::ostringstream oStr;

    oStr << (unsigned char) MSDP_TABLE_OPEN

         << (unsigned char) MSDP_VAR << "AREA"
         << (unsigned char) MSDP_VAL << info.area

         << (unsigned char) MSDP_VAR << "NUM"
         << (unsigned char) MSDP_VAL << info.id

         << (unsigned char) MSDP_VAR << "NAME"
         << (unsigned char) MSDP_VAL << getName();

    if (showExits)
        oStr << getExitsMsdp();

    oStr << (unsigned char) MSDP_TABLE_CLOSE;

    return oStr.str();
}


std::string AreaRoom::getMsdp(bool showExits) const {
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
        oStr << getExitsMsdp();

    oStr << (unsigned char) MSDP_TABLE_CLOSE;
    return oStr.str();
}

std::string Group::getMsdp(Creature* viewer) const {
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


        for(Creature* target : members) {
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
                  << (unsigned char) MSDP_VAL << (showStats ? target->getRoomParent()->getMsdp(false) : "");

            oStr << (unsigned char) MSDP_TABLE_CLOSE;  // Member
        }

        oStr << (unsigned char) MSDP_TABLE_CLOSE;  // Members

    oStr << (unsigned char) MSDP_TABLE_CLOSE; // Group

    return (oStr.str());

}

namespace msdp {
    const std::string UNKNOWN_STR = "unknown";
    const std::string NONE_STR = "none";

    std::string getServerId(Socket &sock, Player* player) {
        return (gConfig->getMudNameAndVersion());
    }

    std::string getServerTime(Socket &sock, Player* player) {
        return (Server::getServerTime());
    }

    const std::string &getCharacterName(Socket &sock, Player* player) {
        return (player ? player->getName() : UNKNOWN_STR);
    }

    std::string getHealth(Socket &sock, Player* player) {
        return (player ? std::to_string(player->hp.getCur()) : UNKNOWN_STR);
    }
    std::string getHealthMax(Socket &sock, Player* player) {
        return (player ? std::to_string(player->hp.getMax()) : UNKNOWN_STR);
    }
    std::string getMana(Socket &sock, Player* player) {
        return (player ? std::to_string(player->mp.getCur()) : UNKNOWN_STR);
    }
    std::string getManaMax(Socket &sock, Player* player) {
        return (player ? std::to_string(player->mp.getMax()) : UNKNOWN_STR);
    }
    std::string getExperience(Socket &sock, Player* player) {
        return (player ? std::to_string(player->getExperience()) : UNKNOWN_STR);
    }
    std::string getExperienceMax(Socket &sock, Player* player) {
        return (player ? player->expNeededDisplay() : UNKNOWN_STR);
    }
    std::string getExperienceTNL(Socket &sock, Player* player) {
        return (player ? std::to_string(player->expToLevel()) : UNKNOWN_STR);
    }
    std::string getExperienceTNLMax(Socket &sock, Player* player) {
        return (player ? player->expForLevel() : UNKNOWN_STR);
    }
    std::string getWimpy(Socket &sock, Player* player) {
        return (player ? std::to_string(player->getWimpy()) : UNKNOWN_STR);
    }
    std::string getMoney(Socket &sock, Player* player) {
        return (player ? player->getCoinDisplay() : UNKNOWN_STR);
    }
    std::string getBank(Socket &sock, Player* player) {
        return (player ? player->getBankDisplay() : UNKNOWN_STR);
    }
    std::string getArmor(Socket &sock, Player* player) {
        return (player ? std::to_string(player->getArmor()) : UNKNOWN_STR);
    }
    std::string getArmorAbsorb(Socket &sock, Player* player) {
        return (player ? std::to_string(floor(player->getDamageReduction(player)*100.0)) : UNKNOWN_STR);
    }
    std::string getGroup(Socket &sock, Player* player) {
        if(player && player->getGroup()) {
            return player->getGroup()->getMsdp(player);
        } else {
            return UNKNOWN_STR;
        }
    }
    const std::string& getTarget(Socket &sock, Player* player) {
        if(player) {
            auto *target = player->getTarget();
            return(target ? target->getName() : NONE_STR);
        } else {
            return UNKNOWN_STR;
        }
    }
    const std::string& getTargetID(Socket &sock, Player* player) {
        if(player) {
            auto *target = player->getTarget();
            return(target ? target->getId() : NONE_STR);
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getTargetHealth(Socket &sock, Player* player) {
        if(player && player->getTarget()) {
            auto *target = player->getTarget();
            return(std::to_string(round((target->hp.getCur()*10000.0) / (target->hp.getMax()*1.0))/100));
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getTargetHealthMax(Socket &sock, Player* player) {
        if(player && player->getTarget()) {
            return std::to_string(100);
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getTargetStrength(Socket &sock, Player* player) {
        if(player && player->getTarget()) {
            // Not implemented
            return UNKNOWN_STR;
        } else {
            return UNKNOWN_STR;
        }
    }
    std::string getRoom(Socket &sock, Player* player) {
        if(player && player->getRoomParent()) {
            return player->getRoomParent()->getMsdp();
        } else
            return UNKNOWN_STR;
    }

};

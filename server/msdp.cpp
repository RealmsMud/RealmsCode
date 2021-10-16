/*
 * Mdsp.h
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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <arpa/telnet.h>          // for IAC, SB, SE
#include <cmath>                  // for floor, round
#include <ostream>                // for operator<<, basic_ostream, basic_os...
#include <string>                 // for operator<<, char_traits, operator!=

#include "bstring.hpp"            // for bstring
#include "config.hpp"             // for Config, MsdpVarMap, gConfig
#include "creatures.hpp"          // for Creature, Player
#include "flags.hpp"              // for P_NO_SHOW_STATS, P_DM_INVIS, P_SLEE...
#include "group.hpp"              // for Group, CreatureList, GROUP_INVITED
#include "login.hpp"              // for CON_DISCONNECTING, CON_PLAYING
#include "msdp.hpp"               // for MsdpVariable, ReportedMsdpVariable
#include "rooms.hpp"              // for AreaRoom, UniqueRoom, BaseRoom, Exi...
#include "server.hpp"             // for Server, SocketList
#include "socket.hpp"             // for Socket, MSDP_VAL, MSDP_VAR, MSDP_TA...

#define MSDP_DEBUG

#define NEW_MSDP_VARIABLE(var, report, player, config, write, interval, sendfn, updatefn, isgrp) \
    msdpVariables[#var] = new MsdpVariable(#var, MSDPVar::var, (report), (player), \
    (config), (write), (interval), (sendfn), (updatefn), (isgrp))


bool Config::initMsdp() {
    //                Name                   Report  Ply  Config W-Once  U  sendFn UpdateFn isGrp
    // Server Info
    NEW_MSDP_VARIABLE(SERVER_ID,             false, false, false, false, 1,  true, false,  false);

    NEW_MSDP_VARIABLE(SERVER_TIME,           false, false, false, false, 1,  true, false,  false);

    // Character Info
    NEW_MSDP_VARIABLE(CHARACTER_NAME,        false,  true, false, false, 1,  true, false,  false);

    NEW_MSDP_VARIABLE(HEALTH,                 true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(HEALTH_MAX,             true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(MANA,                   true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(MANA_MAX,               true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(EXPERIENCE,             true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(EXPERIENCE_MAX,         true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(EXPERIENCE_TNL,         true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(EXPERIENCE_TNL_MAX,     true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(WIMPY,                  true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(MONEY,                  true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(BANK,                   true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(ARMOR,                  true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(ARMOR_ABSORB,           true,  true, false, false, 10, true,  true,  false);

    //  Group

    NEW_MSDP_VARIABLE(GROUP,                  true,  true, false, false, 10, true,  true,  true);


    //  Target
    NEW_MSDP_VARIABLE(TARGET,                 true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(TARGET_ID,              true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(TARGET_HEALTH,          true,  true, false, false, 5,  true,  true,  false);
    NEW_MSDP_VARIABLE(TARGET_HEALTH_MAX,      true,  true, false, false, 10, true,  true,  false);
    NEW_MSDP_VARIABLE(TARGET_STRENGTH,        true,  true, false, false, 10, true,  true,  false);

    // World
    NEW_MSDP_VARIABLE(ROOM,                   true,  true, false, false, 5,  true,  true,  false);

    // Configurable Variables
    NEW_MSDP_VARIABLE(CLIENT_ID,             true,  false,  true,  true, 1, false, false,  false);
    NEW_MSDP_VARIABLE(CLIENT_VERSION,        true,  false,  true,  true, 1, false, false,  false);
    NEW_MSDP_VARIABLE(PLUGIN_ID,             true,  false,  true, false, 1, false, false,  false);
    NEW_MSDP_VARIABLE(ANSI_COLORS,           true,  false,  true, false, 1, false, false,  false);
    NEW_MSDP_VARIABLE(XTERM_256_COLORS,      true,  false,  true, false, 1, false, false,  false);
    NEW_MSDP_VARIABLE(UTF_8,                 true,  false,  true, false, 1, false, false,  false);
    NEW_MSDP_VARIABLE(SOUND,                 true,  false,  true, false, 1, false, false,  false);

    NEW_MSDP_VARIABLE(MXP,                   true,  false,  true, false, 1, false, false,  false);

    return true;
}

void Config::clearMsdp() {
	for(const auto& m : msdpVariables) {
		delete m.second;
	}
	msdpVariables.clear();
}

void Server::processMsdp() {
    for(auto &sock : sockets) {
        if(sock.getState() == CON_DISCONNECTING)
            continue;

        if(sock.mccpEnabled()) {
            for(auto& p : sock.msdpReporting) {
                ReportedMsdpVariable* var = p.second;

                if(var->getRequiresPlayer() && (!sock.getPlayer() || sock.getState() != CON_PLAYING)) continue;

                if(!var->checkTimer()) continue;

                var->update();

                if(!var->isDirty()) continue;

                var->send(sock);

                var->setDirty(false);
            }
        }
    }
}

bool Socket::processMsdpVarVal(bstring& variable, bstring& value) {
#ifdef MSDP_DEBUG
    std::clog << "Found Var: '" << variable << "' Val: '" << value << "'" << std::endl;
#endif
    if (variable.equals("LIST")) {
        return (msdpList(value));
    }
    else if (variable.equals("REPORT")) {
        std::clog << "msdpReport(" << value << ")" << std::endl;
        return (msdpReport(value) != nullptr);
    }
    else if (variable.equals("UNREPORT")) {
        return (msdpUnReport(value));
    }
    else if (variable.equals("SEND")) {
        return (msdpSend(value));
    }
    else {
        // See if they've sent us a configurable variable, if so set it
        auto it = gConfig->msdpVariables.find(variable);
        if(it != gConfig->msdpVariables.end()) {
            MsdpVariable* msdpVar = it->second;

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
        std::clog << "processMsdpVarVal: Unknown variable '" << variable << "'"
                << std::endl;
#endif
    }
    return (true);
}

bool Socket::msdpList(bstring& value) {

    if (value.equals("COMMANDS")) {
        const std::vector<bstring> MsdpCommandList = { "LIST", "REPORT", "RESET", "SEND", "UNREPORT" };
        msdpSendList(value, MsdpCommandList);
        return (true);
    }
    else if (value.equals("LISTS")) {
        const std::vector<bstring> MsdpLists =
                { "COMMANDS", "LISTS", "CONFIGURABLE_VARIABLES", "REPORTABLE_VARIABLES", "REPORTED_VARIABLES", "SENDABLE_VARIABLES" };
        msdpSendList(value, MsdpLists);
        return (true);
    }
    else if (value.equals("SENDABLE_VARIABLES")) {
        std::vector<bstring> sendable;
        for (auto& p: gConfig->msdpVariables) {
            sendable.push_back(p.second->getName());
        }
        msdpSendList(value, sendable);
        return (true);
    }
    else if (value.equals("REPORTABLE_VARIABLES")) {
        std::vector<bstring> reportable;
        for (auto& p : gConfig->msdpVariables) {
            if(p.second->isReportable())
                reportable.push_back(p.second->getName());
        }
        msdpSendList(value, reportable);
        return (true);
    }
    else if (value.equals("CONFIGURABLE_VARIABLES")) {
        std::vector<bstring> configurable;
        for (auto& p : gConfig->msdpVariables) {
            if (p.second->isConfigurable())
                configurable.push_back(p.second->getName());
        }
        msdpSendList(value, configurable);
        return (true);
    }
    else if (value.equals("REPORTED_VARIABLES")) {
        std::vector<bstring> reported;
        for (auto& p : msdpReporting) {
            reported.push_back(p.second->getName());
        }
        msdpSendList("REPORTED_VARIABLES", reported);
        return (true);
    }
    else if (value.equals("")) {
        // If we just get a LIST command, send off a list of all variables
        std::vector<bstring> all;
        for (auto& p : gConfig->msdpVariables) {
            all.push_back(p.second->getName());
        }
        msdpSendList("SENDABLE_VARIABLES", all);
        return (true);
    }

    return (false);
}

ReportedMsdpVariable* Socket::getReportedMsdpVariable(std::string_view value) {
    auto it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (nullptr);
    else
        return (it->second);
}

bstring Socket::getMsdpReporting() {
    std::ostringstream ostr;
    for(auto &report : msdpReporting) {
        ostr << report.first << " ";
    }
    return ostr.str();
}

ReportedMsdpVariable* Socket::msdpReport(bstring& value) {
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

    reported = new ReportedMsdpVariable(msdpVar, this);
    msdpReporting[msdpVar->getName()] = reported;
#ifdef MSDP_DEBUG
    std::clog << "MsdpHandleReport: Now Reporting '" << msdpVar->getName() << "'" << std::endl;
#endif
    return reported;
}

bool Socket::msdpReset(bstring& value) {
    if (value.equals("REPORTABLE_VARIABLES") || value.equals("SENDABLE_VARIABLES")) {
        // For now just clear all reported variables
        msdpClearReporting();
    }
    return (false);
}

void Socket::msdpClearReporting() {
    for (auto& p : msdpReporting) {
        delete (p.second);
    }
    msdpReporting.clear();
}

bool Socket::msdpSend(bstring variable) {
    MsdpVariable *msdpVar = gConfig->getMsdpVariable(variable);
    if(msdpVar == nullptr) {
#ifdef MSDP_DEBUG
        std::clog << "Unknown variable to send: '" << variable << "'" << std::endl;
#endif
        return (false);
    }
    return (msdpVar->send(*this));
}


bool Socket::msdpUnReport(bstring& value) {
    auto it = msdpReporting.find(value);

    if (it == msdpReporting.end())
        return (false);
    else {
#ifdef MSDP_DEBUG
        std::clog << "MsdpHandleUnReport: No longer reporting '" << value << "'" << std::endl;
#endif
        delete it->second;
        msdpReporting.erase(it);
        return (true);
    }
}

void Socket::msdpSendList(std::string_view variable, const std::vector<bstring>& values) {
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
    bstring toSend = oStr.str();

#ifdef MSDP_DEBUG
    debugMsdp(toSend);
#endif

    write(toSend);
    return true;
}

MsdpVariable* Config::getMsdpVariable(bstring& name) {
    auto it = msdpVariables.find(name);
    if(it == msdpVariables.end())
        return(nullptr);
    else
        return(it->second);
}

void MsdpVariable::init() {
    configurable = writeOnce = false;
    requiresPlayer = true;
    reportable = false; // Not reportable by default, only sendable
    updateInterval = 1;
    name ="unknown";
    updateFn = false;
    sendFn = false;
    isGroup = false;
    varId = MSDPVar::UNKNOWN;
}

MsdpVariable::MsdpVariable() {
    init();
}

MsdpVariable::MsdpVariable(const bstring& pName, MSDPVar pVar, bool pReportable, bool pRequiresPlayer, bool pConfigurable,
                           bool pWriteOnce, int pUpdateInterval, bool pSendFn,
                           bool pUpdateFn, bool pIsGroup)
{
    init();
    name = pName;
    varId = pVar;
    reportable = pReportable;
    requiresPlayer = pRequiresPlayer;
    configurable = pConfigurable;
    writeOnce = pWriteOnce;
    updateInterval = pUpdateInterval;
    sendFn = pSendFn;
    updateFn = pUpdateFn;
    isGroup = pIsGroup;
}

ReportedMsdpVariable::ReportedMsdpVariable(const MsdpVariable* mv, Socket* sock) {
    name = mv->getName();
    varId = mv->varId;
    parentSock = sock;
    configurable = mv->isConfigurable();
    writeOnce = mv->isWriteOnce();
    sendFn = mv->sendFn;
    updateFn = mv->updateFn;
    updateInterval = mv->getUpdateInterval();
    reportable = mv->isReportable();
    timer.setDelay(updateInterval);

    dirty = true;
    value = "unknown";
}

bstring MsdpVariable::getName() const {
    return(name);
}

MSDPVar MsdpVariable::getId() const {
    return (varId);
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

bool MsdpVariable::hasSendFn() const {
    return(sendFn);
}

bool MsdpVariable::hasUpdateFn() const {
    return(updateFn);
}

bool MsdpVariable::send(Socket &sock) const {
    bstring value = "";

    if (!hasSendFn()) {
        // If there's no send function, and it's not configurable, there's nothing we can do
        if(!isConfigurable()) return false;

        ReportedMsdpVariable* reported = sock.getReportedMsdpVariable(name);
        if (reported != nullptr) {
            value = reported->getValue();
        }

    } else {
        if (requiresPlayer && !sock.hasPlayer()) return false;

        value = getValue(getId(), sock, sock.getPlayer());
    }

    sock.msdpSendPair(getName(), value);
    return true;
}

bstring ReportedMsdpVariable::getValue() const {
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
    return(setValue(bstring(newValue)));
}

void ReportedMsdpVariable::setValue(long newValue) {
    return(setValue(bstring(newValue)));
}

bool ReportedMsdpVariable::isDirty() const {
    return(dirty);
}

void ReportedMsdpVariable::update() {
    if(!hasUpdateFn()) return;
    if(!parentSock) return;

    bstring oldValue = value;
    setValue(MsdpVariable::getValue(getId(), *parentSock, parentSock->getPlayer()));
}

void ReportedMsdpVariable::setDirty(bool pDirty) {
    dirty = pDirty;
}

bstring BaseRoom::getExitsMsdp() const {
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

bstring UniqueRoom::getMsdp(bool showExits) const {
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


bstring AreaRoom::getMsdp(bool showExits) const {
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

bstring Group::getMsdp(Creature* viewer) const {
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

bstring MsdpVariable::getValue(MSDPVar var, Socket &sock, Player* player) {
    switch(var) {

        case MSDPVar::SERVER_ID:
            return (gConfig->getMudNameAndVersion());
        case MSDPVar::SERVER_TIME:
            return (Server::getServerTime());

        case MSDPVar::CHARACTER_NAME:
            if (player) return (player->getName());
            break;

        case MSDPVar::HEALTH:
            if (player) return (player->hp.getCur());
            break;
        case MSDPVar::HEALTH_MAX:
            if (player) return (player->hp.getMax());
            break;

        case MSDPVar::MANA:
            if (player) return (player->mp.getCur());
            break;
        case MSDPVar::MANA_MAX:
            if (player) return (player->mp.getMax());
            break;

        case MSDPVar::EXPERIENCE:
            if (player) return (player->getExperience());
            break;
        case MSDPVar::EXPERIENCE_MAX:
            if (player) return (player->expNeededDisplay());
            break;
        case MSDPVar::EXPERIENCE_TNL:
            if (player) return (player->expToLevel());
            break;
        case MSDPVar::EXPERIENCE_TNL_MAX:
            if (player) return (player->expForLevel());
            break;

        case MSDPVar::WIMPY:
            if (player) return (player->getWimpy());
            break;

        case MSDPVar::MONEY:
            if (player) return (player->getCoinDisplay());
            break;
        case MSDPVar::BANK:
            if (player) return (player->getBankDisplay());
            break;

        case MSDPVar::ARMOR:
            if (player) return (player->getArmor());
            break;
        case MSDPVar::ARMOR_ABSORB:
            if (player) return (floor(player->getDamageReduction(player)*100.0));
            break;

        case MSDPVar::TARGET:
            if (player) {
                Creature *target = player->getTarget();
                if (target)
                    return (target->getName());
                else
                    return "none";
            }
            break;
        case MSDPVar::TARGET_ID:
            if (player) {
                Creature *target = player->getTarget();
                if (target)
                    return (target->getId());
                else
                    return "none";
            }
            break;
        case MSDPVar::TARGET_HEALTH:
            if (player) {
                Creature* target = player->getTarget();
                if (target) return (round((target->hp.getCur()*10000.0) / (target->hp.getMax()*1.0))/100);
            }
            break;
        case MSDPVar::TARGET_HEALTH_MAX:
            if (player) {
                if (player->getTarget()) return (100);
            }
            break;
        case MSDPVar::TARGET_STRENGTH:
            if (player) {
                if (player->getTarget()) return ("Unknown");
            }
            break;

        case MSDPVar::ROOM:
            if (player) {
                BaseRoom* room = player->getRoomParent();
                if (room) return (room->getMsdp());
            }
            break;

        case MSDPVar::GROUP:
            if (player) {
                Group* group = player->getGroup();
                if (group) return group->getMsdp(player);
            }
            break;

        default:
            break;
    }
    return "unknown";
}

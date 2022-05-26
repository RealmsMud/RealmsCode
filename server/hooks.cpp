/*
 * hooks.cpp
 *   Dynamic hooks
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

#include <fmt/format.h>              // for format
#include <map>                       // for map, operator==, _Rb_tree_const_...
#include <proto.hpp>                 // for broadcast
#include <sstream>                   // for operator<<, basic_ostream, ostri...
#include <string>                    // for string, allocator, operator+
#include <string_view>               // for string_view
#include <utility>                   // for pair

#include "area.hpp"                  // for MapMarker
#include "flags.hpp"                 // for P_SEE_ALL_HOOKS, P_SEE_HOOKS
#include "hooks.hpp"                 // for Hooks
#include "mudObjects/areaRooms.hpp"  // for AreaRoom
#include "mudObjects/mudObject.hpp"  // for MudObject
#include "mudObjects/players.hpp"    // for Player
#include "server.hpp"                // for Server, gServer
#include "socket.hpp"                // for Socket
#include "mud.hpp"


//*********************************************************************
//                      Hook
//*********************************************************************
// View the "hooks" dm help file to view available hooks.

//Hooks::Hooks() = default;

Hooks::Hooks(MudObject* target) {
    setParent(target);
}

//*********************************************************************
//                      doCopy
//*********************************************************************

void Hooks::doCopy(const Hooks& h) {
    hooks.clear();

    for(const auto& p : h.hooks) {
        add(p.first, p.second);
    }
}

Hooks& Hooks::operator=(const Hooks& h) {
    if (this == &h)
        return (*this);

    doCopy(h);
    return(*this);
}

//*********************************************************************
//                      Hook
//*********************************************************************

void Hooks::setParent(MudObject* target) {
    parent = target;
}

//*********************************************************************
//                      add
//*********************************************************************

void Hooks::add(std::string_view event, std::string_view code) {
    hooks.insert(std::pair<std::string,std::string>(event, code));
}



//*********************************************************************
//                      display
//*********************************************************************

std::string Hooks::display() const {
    if(hooks.empty())
        return("");

    std::ostringstream oStr;

    oStr << "^oHooks:^x\n";
    for( const auto& p : hooks ) {
        oStr << "^WEvent:^x " << p.first << "\n^WCode:^x" << p.second << "\n";
    }

    return(oStr.str());
}

//*********************************************************************
//                      seeHooks
//*********************************************************************

bool seeHooks(std::shared_ptr<Socket> sock) {
    if(sock->getPlayer())
        return(sock->getPlayer()->isDm() && (
            sock->getPlayer()->flagIsSet(P_SEE_HOOKS) ||
            sock->getPlayer()->flagIsSet(P_SEE_ALL_HOOKS)
        ));
    return(false);
}

//*********************************************************************
//                      seeAllHooks
//*********************************************************************

bool seeAllHooks(std::shared_ptr<Socket> sock) {
    if(sock->getPlayer())
        return(sock->getPlayer()->isDm() && sock->getPlayer()->flagIsSet(P_SEE_ALL_HOOKS));
    return(false);
}

//*********************************************************************
//                      hookMudObjName
//*********************************************************************

std::string hookMudObjName(const MudObject* target) {
    if(!target)
        return("^W-none-^o");
    const std::shared_ptr<const AreaRoom> aRoom = target->getAsConstAreaRoom();
    if(!aRoom)
        return((std::string)target->getName() + "^o");
    return(aRoom->mapmarker.str() + "^o");
}

std::string hookMudObjName(const std::shared_ptr<MudObject>& target) {
    return hookMudObjName(target.get());
}

//*********************************************************************
//                      execute
//*********************************************************************

bool Hooks::execute(const std::string &event, const std::shared_ptr<MudObject>& target, const std::string &param1, const std::string &param2, const std::string &param3) const {
    // No hooks while crashing
    if(Crash) return false;

    bool ran = false;

    std::string params;
    if(!param1.empty())
        params += "   param1: " + param1;
    if(!param2.empty())
        params += "   param2: " + param2;
    if(!param3.empty())
        params += "   param3: " + param3;

     broadcast(seeAllHooks, fmt::format("^ochecking hook {}: {}^o on {}^o{}", event,
        hookMudObjName(parent), hookMudObjName(target), params).c_str());

    //std::unordered_map<std::string, std::string>::const_iterator it = hooks.find(event);
    auto it = hooks.find(event);


    if(it != hooks.end()) {
        ran = true;

        broadcast(seeHooks, fmt::format("^orunning hook {}: {}^o on {}^o{}: ^x{}", event,
            hookMudObjName(parent), hookMudObjName(target), params, it->second).c_str());
        gServer->runPython(it->second, param1 + "," + param2 + "," + param3, parent->shared_from_this(), target);
    }
    return(ran);
}

bool Hooks::executeWithReturn(const std::string &event, const std::shared_ptr<MudObject>& target, const std::string &param1, const std::string &param2, const std::string &param3) const {
    // No hooks while crashing
    if(Crash) return false;

    bool returnValue = true;

    std::string params;
    if(!param1.empty())
        params += "   param1: " + param1;
    if(!param2.empty())
        params += "   param2: " + param2;
    if(!param3.empty())
        params += "   param3: " + param3;

    broadcast(seeAllHooks, fmt::format("^ochecking hook {}: {}^o on {}^o{}", event,
        hookMudObjName(parent), hookMudObjName(target), params).c_str());

    auto it = hooks.find(event);


    if(it != hooks.end()) {
        broadcast(seeHooks, fmt::format("^orunning hook {}: {}^o on {}^o{}: ^x", event,
            hookMudObjName(parent), hookMudObjName(target), params).c_str());

        returnValue = gServer->runPythonWithReturn(it->second, param1 + "," + param2 + "," + param3, parent->shared_from_this(), target);
    }
    return(returnValue);
}
//*********************************************************************
//                      execute
//*********************************************************************
// For hooks that must be run in pairs, run this

// A trigger1 or trigger2 null value is valid, so handle appropriate
bool Hooks::run(const std::shared_ptr<MudObject>& trigger1, const std::string &event1, const std::shared_ptr<MudObject>& trigger2, const std::string &event2, const std::string &param1, const std::string &param2, const std::string &param3) {
    // No hooks while crashing
    if(Crash) return false;

    bool ran=false;
    if(trigger1 && trigger1->hooks.execute(event1, trigger2, param1, param2, param3))
        ran = true;
    if(trigger2 && trigger2->hooks.execute(event2, trigger1, param1, param2, param3))
        ran = true;
    return(ran);
}

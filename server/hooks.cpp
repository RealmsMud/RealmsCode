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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <proto.hpp>              // for broadcast
#include <map>                    // for operator==, operator!=
#include <sstream>                // for operator<<, basic_ostream, char_traits
#include <string>                 // for operator<<, allocator
#include <utility>                // for pair

#include "bstring.hpp"            // for bstring, operator+
#include "creatures.hpp"          // for Player
#include "flags.hpp"              // for P_SEE_ALL_HOOKS, P_SEE_HOOKS
#include "hooks.hpp"              // for Hooks
#include "mudObject.hpp"          // for MudObject
#include "rooms.hpp"              // for AreaRoom
#include "server.hpp"             // for Server, gServer
#include "socket.hpp"             // for Socket


//*********************************************************************
//                      Hook
//*********************************************************************
// View the "hooks" dm help file to view available hooks.

Hooks::Hooks() {
    parent = nullptr;
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

void Hooks::add(const bstring& event, const bstring& code) {
    hooks.insert(std::pair<bstring,bstring>(event, code));
}



//*********************************************************************
//                      display
//*********************************************************************

bstring Hooks::display() const {
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

bool seeHooks(Socket* sock) {
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

bool seeAllHooks(Socket* sock) {
    if(sock->getPlayer())
        return(sock->getPlayer()->isDm() && sock->getPlayer()->flagIsSet(P_SEE_ALL_HOOKS));
    return(false);
}

//*********************************************************************
//                      hookMudObjName
//*********************************************************************

bstring hookMudObjName(const MudObject* target) {
    if(!target)
        return("^W-none-^o");
    const AreaRoom* aRoom = target->getAsConstAreaRoom();
    if(!aRoom)
        return((bstring)target->getName() + "^o");
    return(aRoom->mapmarker.str() + "^o");
}

//*********************************************************************
//                      execute
//*********************************************************************

bool Hooks::execute(const bstring& event, MudObject* target, const bstring& param1, const bstring& param2, const bstring& param3) const {
    bool ran = false;

    bstring params = "";
    if(!param1.empty())
        params += "   param1: " + param1;
    if(!param2.empty())
        params += "   param2: " + param2;
    if(!param3.empty())
        params += "   param3: " + param3;

    broadcast(seeAllHooks, "^ochecking hook %s: %s^o on %s^o%s", event.c_str(),
        hookMudObjName(parent).c_str(), hookMudObjName(target).c_str(),
        params.c_str());

    //std::unordered_map<bstring, bstring>::const_iterator it = hooks.find(event);
    auto it = hooks.find(event);


    if(it != hooks.end()) {
        ran = true;

        broadcast(seeHooks, "^orunning hook %s: %s^o on %s^o%s: ^x%s", event.c_str(),
            hookMudObjName(parent).c_str(), hookMudObjName(target).c_str(),
            params.c_str(), it->second.c_str());
        gServer->runPython(it->second, param1 + "," + param2 + "," + param3, parent, target);
    }
    return(ran);
}

bool Hooks::executeWithReturn(const bstring& event, MudObject* target, const bstring& param1, const bstring& param2, const bstring& param3) const {
    bool returnValue = true;

    bstring params = "";
    if(!param1.empty())
        params += "   param1: " + param1;
    if(!param2.empty())
        params += "   param2: " + param2;
    if(!param3.empty())
        params += "   param3: " + param3;

    broadcast(seeAllHooks, "^ochecking hook %s: %s^o on %s^o%s", event.c_str(),
        hookMudObjName(parent).c_str(), hookMudObjName(target).c_str(),
        params.c_str());

    auto it = hooks.find(event);


    if(it != hooks.end()) {
        broadcast(seeHooks, "^orunning hook %s: %s^o on %s^o%s: ^x", event.c_str(),
            hookMudObjName(parent).c_str(), hookMudObjName(target).c_str(),
            params.c_str());

        returnValue = gServer->runPythonWithReturn(it->second, param1 + "," + param2 + "," + param3, parent, target);
    }
    return(returnValue);
}
//*********************************************************************
//                      execute
//*********************************************************************
// For hooks that must be run in pairs, run this

// A trigger1 or trigger2 null value is valid, so handle appropriate
bool Hooks::run(MudObject* trigger1, const bstring& event1, MudObject* trigger2, const bstring& event2, const bstring& param1, const bstring& param2, const bstring& param3) {
    bool ran=false;
    if(trigger1 && trigger1->hooks.execute(event1, trigger2, param1, param2, param3))
        ran = true;
    if(trigger2 && trigger2->hooks.execute(event2, trigger1, param1, param2, param3))
        ran = true;
    return(ran);
}

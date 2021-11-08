/*
 * proxy.cpp
 *   Source file for proxy access
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


#include <boost/format.hpp>             // for basic_altstringbuf<>::int_type
#include <cctype>                       // for isdigit
#include <boost/algorithm/string/case_conv.hpp>

#include "cmd.hpp"                      // for cmd
#include "config.hpp"                   // for Config, gConfig
#include "creatures.hpp"                // for Player
#include "free_crt.hpp"                 // for free_crt
#include "proto.hpp"                    // for up, file_exists
#include "proxy.hpp"                    // for ProxyAccess, ProxyManager
#include "server.hpp"                   // for Server, gServer
#include "xml.hpp"                      // for newStringChild, copyToString

//*********************************************************************
// CmdProxy
//*********************************************************************
// Allows a player to view, grant, or revoke proxy access
// to their character

int cmdProxy(Player* player, cmd* cmnd) {

    if(cmnd->num == 1) {
        std::string proxyList = gConfig->getProxyList(player);
        if(proxyList.empty())
            *player << "No proxy access currently allowed.\n";
        else
            *player << proxyList;
        return(0);
    }
    if(!player->getProxyName().empty()) {
        *player << "You cannot change the proxy access of a proxied character.\n";
        return(0);
    }
    if(player->isStaff()) {
        *player << "You cannot allow proxy access to a staff character!\n";
        return(0);
    }
    bool online=true;

    Player* target=nullptr;
    std::string name =  cmnd->str[1];

    if(name.length() > 1 && name.at(0) == 'p' && isdigit(name.at(1))) {
        target = gServer->lookupPlyId(name);
    }

    if(!target && name.length() > 1 && name.at(0) == 'p' && isdigit(name.at(1))) {
        // Dealing with an ID
        name.at(0) = up(name.at(0));
        // TODO: Print success/failure

        if(gConfig->removeProxyAccess(name, player) )
            *player << "Proxy access removed for PlayerID: " << name << "\n";
        else
            *player << "PlayerID: " << name << " does not have proxy access to your character.\n";
        return(0);
    }
    else {
        // We're dealing with a player
        if(!target) {
            boost::to_lower(name);
            name.at(0) = up(name.at(0));
            target = gServer->findPlayer(name);
        }

        if(!target) {
            if(!loadPlayer(name.c_str(), &target)) {
                *player << "Player does not exist.\n";
                return(0);
            }

            online = false;
        }
        if(target->isStaff()) {
            if(!online)
                free_crt(target);
            *player << "You cannot give staff proxy access!.\n";
            return(0);
        }
        if(target->getId() == player->getId()) {
            *player << "That's just silly.\n";
            if(!online)
                free_crt(target);
            return(0);
        }

        if(gConfig->hasProxyAccess(target, player)) {
            *player << "You remove " << target->getName() << "'s proxy access to your character.\n";

            gConfig->removeProxyAccess(target, player);
            if(online)
                *target << "Your proxy access to " << player->getName() << " has been revoked.\n";
            else
                free_crt(target);
        } else {
            *player << "You grant " << target->getName() << " proxy access to your character.\n";

            gConfig->grantProxyAccess(target, player);
            if(online)
                *target << "Your now have been granted proxy access to " << player->getName() << ".\n";
            else
                free_crt(target);

        }
    }
    return(0);
}

//*********************************************************************
//                      Config proxy functions
//*********************************************************************

//*********************************************************************
// HasProxyAccess
//*********************************************************************
// Does (proxy) have access to (proxied)

bool Config::hasProxyAccess(Player* proxy, Player* proxied) {
    return(proxyManager->hasProxyAccess(proxy, proxied));
}

//*********************************************************************
// getProxyList
//*********************************************************************
// Returns a std::string showing proxy access for the given player
// or all players if player is null

std::string Config::getProxyList(Player* player) {
    std::ostringstream oStr;
    std::string lastId = "";

    oStr.setf(std::ios::left, std::ios::adjustfield);
    boost::format format("%1% %|15t|%2% %|45t|%3%\n");

    oStr << format % "" % "Character Name" % "Proxy Access";
    for(const ProxyMultiMap::value_type& p : proxyManager->proxies) {
        auto& proxy = p.second;

        if(player && proxy.getProxiedId() != player->getId())
            continue;

        format.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )  );
        if(proxy.getProxiedId() != lastId) {
            lastId = proxy.getProxiedId();
            oStr << format % "Character: "
                           % (proxy.getProxiedName() + "(" + proxy.getProxiedId() + ")")
                           % (std::string("- ") + proxy.getProxyName() + "(" + proxy.getProxyId() + ")");
        } else {
            oStr << format % ""
                           % ""
                           % (std::string("- ") + proxy.getProxyName() + "(" + proxy.getProxyId() + ")");
        }
    }
    return(oStr.str());
}

//*********************************************************************
// GrantProxyAccess
//*********************************************************************
// Grant proxy access to (proxied) for (proxy)

void Config::grantProxyAccess(Player* proxy, Player* proxied) {
    proxyManager->grantProxyAccess(proxy, proxied);
}

//*********************************************************************
// RemoveProxyAccess
//*********************************************************************
// Remove proxy access to (proxied) for (proxy)

bool Config::removeProxyAccess(Player* proxy, Player* proxied) {
    return(proxyManager->removeProxyAccess(proxy, proxied));
}
//*********************************************************************
// RemoveProxyAccess
//*********************************************************************
// Remove proxy access to (proxied) for (id)

bool Config::removeProxyAccess(std::string_view id, Player* proxied) {
    return(proxyManager->removeProxyAccess(id, proxied));
}


//*********************************************************************
// LoadProxyAccess
//*********************************************************************
// Reset the proxy manager if necessary and load in proxy access

void Config::loadProxyAccess() {
    if(proxyManager)
        proxyManager->clear();
    proxyManager = new ProxyManager;
}
//*********************************************************************
// SaveProxyAccess
//*********************************************************************
// Save proxy access list

void Config::saveProxyAccess() {
    proxyManager->save();
}

//*********************************************************************
//                      Player proxy functions
//*********************************************************************

//*********************************************************************
// SetProxy
//*********************************************************************
// Set's player as the current proxy user

void Player::setProxy(Player* proxy) {
    proxyName = proxy->getName();
    proxyId = proxy->getId();
}

//*********************************************************************
// SetProxy
//*********************************************************************
// Set's proxyName & proxyId as the current proxy user & ID

void Player::setProxy(std::string_view pProxyName, std::string_view pProxyId) {
    proxyName = pProxyName;
    proxyId = pProxyId;
}

//*********************************************************************
// SetProxyName
//*********************************************************************
// Set's proxyName as the current proxy user

void Player::setProxyName(std::string_view pProxyName) {
    proxyName = pProxyName;
}

//*********************************************************************
// SetProxyID
//*********************************************************************
// Set's proxyId as the current proxy ID

void Player::setProxyId(std::string_view pProxyId) {
    proxyId = pProxyId;
}


std::string Player::getProxyName() const { return(proxyName); }
std::string Player::getProxyId() const { return(proxyId); }



//*********************************************************************
//                ProxyManager proxy functions
//*********************************************************************

ProxyManager::ProxyManager() {
    loadProxies();
}

void ProxyManager::clear() {
    proxies.clear();
    delete this;
}

//*********************************************************************
// grantProxyAccess
//*********************************************************************
// Grant proxy access to (proxied) for (proxy)

void ProxyManager::grantProxyAccess(Player* proxy, Player* proxied) {
    if(hasProxyAccess(proxy, proxied))
        return;
    proxies.insert(ProxyMultiMap::value_type(proxied->getId(), ProxyAccess(proxy, proxied)));
    gConfig->saveProxyAccess();
}

//*********************************************************************
// removeProxyAccess
//*********************************************************************
// Remove proxy access to (proxied) for (proxy)

bool ProxyManager::removeProxyAccess(Player* proxy, Player* proxied) {
    return(removeProxyAccess(proxy->getId(), proxied));
}

//*********************************************************************
// removeProxyAccess
//*********************************************************************
// Remove proxy access to (proxied) for (proxy)

bool ProxyManager::removeProxyAccess(std::string_view id, Player* proxied) {
    ProxyMultiMapRange range = proxies.equal_range(proxied->getId());
    for(auto it = range.first ; it != range.second ; it++) {
        ProxyAccess& proxyAccess = it->second;
        if(proxyAccess.getProxyId() == id) {
            proxies.erase(it);
            gConfig->saveProxyAccess();
            return(true);
        }
    }
    return(false);
}

//*********************************************************************
// hasProxyAccess
//*********************************************************************
// True if (proxy) has proxy access to (proxied)

bool ProxyManager::hasProxyAccess(Player* proxy, Player* proxied) {
    if(!proxy || !proxied)
        return(false);

    ProxyMultiMapRange range = proxies.equal_range(proxied->getId());
    for(auto it = range.first ; it != range.second ; it++) {
        ProxyAccess& proxyAccess = it->second;
        if(proxyAccess.hasProxyAccess(proxy, proxied))
            return(true);
    }
    return(false);
}


//*********************************************************************
//                ProxyAccess proxy functions
//*********************************************************************

ProxyAccess::ProxyAccess(Player* proxy, Player* proxied) {
    proxyName = proxy->getName();
    proxyId = proxy->getId();
    proxiedName = proxied->getName();
    proxiedId = proxied->getId();
}

//*********************************************************************
// hasProxyAccess
//*********************************************************************
// True if (proxy) has proxy access to (proxied)

bool ProxyAccess::hasProxyAccess(Player* proxy, Player* proxied) {
    if(proxiedId == proxied->getId() && proxiedName == proxied->getName() &&
            proxyId == proxy->getId() && proxyName == proxy->getName())
        return(true);
    return(false);
}
std::string ProxyAccess::getProxiedId() const { return proxiedId; }
std::string ProxyAccess::getProxiedName() const { return proxiedName; }
std::string ProxyAccess::getProxyId() const { return proxyId; }

std::string ProxyAccess::getProxyName() const { return proxyName; }




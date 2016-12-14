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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"

#include <sstream>
#include <iomanip>
#include <locale>
#include <iostream>
#include <fstream>

#include "boost/format.hpp"

//*********************************************************************
// CmdProxy
//*********************************************************************
// Allows a player to view, grant, or revoke proxy access
// to their character

int cmdProxy(Player* player, cmd* cmnd) {

    if(cmnd->num == 1) {
        bstring proxyList = gConfig->getProxyList(player);
        if(proxyList.empty())
            *player << "No proxy access currently allowed.\n";
        else
            *player << proxyList;
        return(0);
    }
    if(player->getProxyName() != "") {
        *player << "You cannot change the proxy access of a proxied character.\n";
        return(0);
    }
    if(player->isStaff()) {
        *player << "You cannot allow proxy access to a staff character!\n";
        return(0);
    }
    bool online=true;

    Player* target=0;
    bstring name =  cmnd->str[1];

    if(name.length() > 1 && name.getAt(0) == 'p' && isdigit(name.getAt(1))) {
        target = gServer->lookupPlyId(name);
    }

    if(!target && name.length() > 1 && name.getAt(0) == 'p' && isdigit(name.getAt(1))) {
        // Dealing with an ID
        name.setAt(0, up(name.getAt(0)));
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
            name = name.toLower();
            name.setAt(0, up(name.getAt(0)));
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
// Returns a bstring showing proxy access for the given player
// or all players if player is null

bstring Config::getProxyList(Player* player) {
    std::ostringstream oStr;
    bstring lastId = "";

    oStr.setf(std::ios::left, std::ios::adjustfield);
    boost::format format("%1% %|15t|%2% %|45t|%3%\n");

    oStr << format % "" % "Character Name" % "Proxy Access";
    for(ProxyMultiMap::value_type p : proxyManager->proxies) {
        ProxyAccess &proxy = p.second;

        if(player && proxy.getProxiedId() != player->getId())
            continue;

        format.exceptions( boost::io::all_error_bits ^ ( boost::io::too_many_args_bit | boost::io::too_few_args_bit )  );
        if(proxy.getProxiedId() != lastId) {
            lastId = proxy.getProxiedId();
            oStr << format % "Character: "
                           % (proxy.getProxiedName() + "(" + proxy.getProxiedId() + ")")
                           % (bstring("- ") + proxy.getProxyName() + "(" + proxy.getProxyId() + ")");
        } else {
            oStr << format % ""
                           % ""
                           % (bstring("- ") + proxy.getProxyName() + "(" + proxy.getProxyId() + ")");
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

bool Config::removeProxyAccess(bstring id, Player* proxied) {
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

void Player::setProxy(bstring pProxyName, bstring pProxyId) {
    proxyName = pProxyName;
    proxyId = pProxyId;
}

//*********************************************************************
// SetProxyName
//*********************************************************************
// Set's proxyName as the current proxy user

void Player::setProxyName(bstring pProxyName) {
    proxyName = pProxyName;
}

//*********************************************************************
// SetProxyID
//*********************************************************************
// Set's proxyId as the current proxy ID

void Player::setProxyId(bstring pProxyId) {
    proxyId = pProxyId;
}


bstring Player::getProxyName() const { return(proxyName); }
bstring Player::getProxyId() const { return(proxyId); }



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

bool ProxyManager::removeProxyAccess(bstring id, Player* proxied) {
    ProxyMultiMapRange range = proxies.equal_range(proxied->getId());
    for(ProxyMultiMap::iterator it = range.first ; it != range.second ; it++) {
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
    for(ProxyMultiMap::iterator it = range.first ; it != range.second ; it++) {
        ProxyAccess& proxyAccess = it->second;
        if(proxyAccess.hasProxyAccess(proxy, proxied))
            return(true);
    }
    return(false);
}


void ProxyManager::save() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr      rootNode;
    char            filename[80];


    xmlDoc = xmlNewDoc(BAD_CAST "1.0");
    rootNode = xmlNewDocNode(xmlDoc, nullptr, BAD_CAST "Proxies", nullptr);
    xmlDocSetRootElement(xmlDoc, rootNode);

    for(ProxyMultiMap::value_type p : proxies) {
        p.second.save(rootNode);
    }

    sprintf(filename, "%s/proxies.xml", Path::PlayerData);

    xml::saveFile(filename, xmlDoc);
    xmlFreeDoc(xmlDoc);
}

void ProxyManager::loadProxies() {
    xmlDocPtr   xmlDoc;
    xmlNodePtr  rootNode;
    xmlNodePtr  curNode;
    char        filename[80];

    sprintf(filename, "%s/proxies.xml", Path::PlayerData);

    if(!file_exists(filename))
        return;

    if((xmlDoc = xml::loadFile(filename, "Proxies")) == nullptr)
        return;

    rootNode = xmlDocGetRootElement(xmlDoc);
    curNode = rootNode->children;

    while(curNode) {
        if(NODE_NAME(curNode, "ProxyAccess")) {
            ProxyAccess access(curNode);
            proxies.insert(ProxyMultiMap::value_type(access.getProxiedId(), access));
        }
        curNode = curNode->next;
    }

    xmlFreeDoc(xmlDoc);
    xmlCleanupParser();
}


//*********************************************************************
//                ProxyAccess proxy functions
//*********************************************************************



ProxyAccess::ProxyAccess(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;
    while(curNode) {
             if(NODE_NAME(curNode, "ProxyId")) xml::copyToBString(proxyId, curNode);
        else if (NODE_NAME(curNode, "ProxyName")) xml::copyToBString(proxyName, curNode);
        else if (NODE_NAME(curNode, "ProxiedId")) xml::copyToBString(proxiedId, curNode);
        else if (NODE_NAME(curNode, "ProxiedName")) xml::copyToBString(proxiedName, curNode);

        curNode = curNode->next;
    }
}
ProxyAccess::ProxyAccess(Player* proxy, Player* proxied) {
    proxyName = proxy->getName();
    proxyId = proxy->getId();
    proxiedName = proxied->getName();
    proxiedId = proxied->getId();
}

void ProxyAccess::save(xmlNodePtr rootNode) {
    xmlNodePtr curNode = xml::newStringChild(rootNode, "ProxyAccess");
    xml::newStringChild(curNode, "ProxiedId", proxiedId);
    xml::newStringChild(curNode, "ProxiedName", proxiedName);
    xml::newStringChild(curNode, "ProxyId", proxyId);
    xml::newStringChild(curNode, "ProxyName", proxyName);

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
bstring ProxyAccess::getProxiedId() const { return proxiedId; }
bstring ProxyAccess::getProxiedName() const { return proxiedName; }
bstring ProxyAccess::getProxyId() const { return proxyId; }

bstring ProxyAccess::getProxyName() const { return proxyName; }




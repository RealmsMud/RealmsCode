/*
 * proxy.h
 *   Header file for proxy access
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

#include <map>
#include <libxml/parser.h>  // for xmlNodePtr


class Player;
class ProxyAccess;

typedef std::multimap<std::string, ProxyAccess> ProxyMultiMap;
typedef std::pair<ProxyMultiMap::iterator, ProxyMultiMap::iterator> ProxyMultiMapRange;

class ProxyManager {
    friend class Config;
public:
    ProxyManager();
    void save();
    void clear();
    bool hasProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    void grantProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    bool removeProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    bool removeProxyAccess(std::string_view id, std::shared_ptr<Player> proxied);

protected:
    void loadProxies();
    ProxyMultiMap proxies;
};

class ProxyAccess {
public:
    ProxyAccess(xmlNodePtr rootNode);
    ProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    bool hasProxyAccess(std::shared_ptr<Player> proxy, std::shared_ptr<Player> proxied);
    void save(xmlNodePtr rootNode);
protected:
    // Character allowing proxy access
    std::string proxiedName;
    std::string proxiedId;

    // Character allowed to proxy
    std::string proxyName;
    std::string proxyId;

public:
    [[nodiscard]] std::string getProxiedId() const;
    [[nodiscard]] std::string getProxiedName() const;
    [[nodiscard]] std::string getProxyId() const;
    [[nodiscard]] std::string getProxyName() const;
};


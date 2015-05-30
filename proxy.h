/*
 * proxy.h
 *	 Header file for proxy access
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef PROXY_H_
#define PROXY_H_

class ProxyAccess;

typedef std::multimap<bstring, ProxyAccess> ProxyMultiMap;
typedef std::pair<ProxyMultiMap::iterator, ProxyMultiMap::iterator> ProxyMultiMapRange;

class ProxyManager {
	friend class Config;
public:
	ProxyManager();
	void save();
	void clear();
	bool hasProxyAccess(Player* proxy, Player* proxied);
	void grantProxyAccess(Player* proxy, Player* proxied);
	bool removeProxyAccess(Player* proxy, Player* proxied);
	bool removeProxyAccess(bstring id, Player* proxied);

protected:
	void loadProxies();
	ProxyMultiMap proxies;
};

class ProxyAccess {
public:
	ProxyAccess(xmlNodePtr rootNode);
	ProxyAccess(Player* proxy, Player* proxied);
	bool hasProxyAccess(Player* proxy, Player* proxied);
	void save(xmlNodePtr rootNode);
protected:
	// Character allowing proxy access
	bstring proxiedName;
	bstring proxiedId;

	// Character allowed to proxy
	bstring proxyName;
	bstring proxyId;

public:
	bstring getProxiedId() const;
	bstring getProxiedName() const;
	bstring getProxyId() const;
	bstring getProxyName() const;
};


#endif /* PROXY_H_ */

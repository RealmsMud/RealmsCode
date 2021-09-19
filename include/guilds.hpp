/*
 * guilds.h
 *   Header file for guilds
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
#ifndef GUILDS_H_
#define GUILDS_H_

#include <map>
#include <list>

#include "money.hpp"

class cmd;
class Player;
class Property;
class UniqueRoom;

bool shopStaysWithGuild(const UniqueRoom* shop);


class GuildCreation {
public:
    GuildCreation();

    bool addSupporter(Player* supporter);
    bool removeSupporter(const bstring& supporterName);
    void renameSupporter(const bstring& oldName, const bstring& newName);
    bool saveToXml(xmlNodePtr rootNode) const;

public:
    bstring name;
    bstring leader;
    bstring leaderIp;
    int     status;
    int     numSupporters;
    std::map<bstring, bstring> supporters;
};


class Guild {
public:
    Guild();
    explicit Guild(xmlNodePtr curNode);
    void recalcLevel();
    int averageLevel();
    bool addMember(const bstring& memberName);
    bool delMember(const bstring& memberName);
    bool isMember(const bstring& memberName);
    void renameMember(const bstring& oldName, const bstring& newName);
    void parseGuildMembers(xmlNodePtr cur);
    bool saveToXml(xmlNodePtr rootNode) const;

    static void create(Player* player, cmd* cmnd);
    static void forum(Player* player, cmd* cmnd);
    static void cancel(Player* player, cmd* cmnd);
    static void support(Player* player, cmd* cmnd);
    static void invite(Player* player, cmd* cmnd);
    static void abdicate(Player* player, cmd* cmnd);
    static void abdicate(Player* player, Player* target, bool online=true);
    static void remove(Player* player, cmd* cmnd);
    static void join(Player* player, cmd *cmnd);
    static void reject(Player* player, cmd* cmnd);
    static void disband(Player* player, cmd* cmnd);

    static void list(Player* player, cmd* cmnd);
    static void viewMembers(Player* player, cmd* cmnd);
    static void promote(Player* player, cmd* cmnd);
    static void demote(Player* player, cmd* cmnd);


public:
    std::list<bstring> members;
    Money   bank;

    [[nodiscard]] bstring getName() const;
    [[nodiscard]] unsigned short getNum() const;
    [[nodiscard]] bstring getLeader() const;
    [[nodiscard]] long getLevel() const;
    [[nodiscard]] int getNumMembers() const;
    [[nodiscard]] long getPkillsIn() const;
    [[nodiscard]] long getPkillsWon() const;
    [[nodiscard]] long getPoints() const;

    void setName(const bstring& n);
    void setNum(unsigned short n);
    void setLeader(const bstring& l);
    void setLevel(long l);
    void setNumMembers(int n);
    void setPkillsIn(long pk);
    void setPkillsWon(long pk);
    void setPoints(long p);

    void incLevel(int l=1);
    void incNumMembers(int n=1);
    void incPkillsIn(long pk=1);
    void incPkillsWon(long pk=1);

    void guildhallLocations(const Player* player, const char* fmt) const;
protected:
    bstring name;
    unsigned short num;
    bstring leader;
    long    level{};      // Sum of everyone's level in the guild
    int     numMembers{}; // Number of members in the guild
    long    pkillsIn{};
    long    pkillsWon{};
    long    points{};     // Points from quests/etc
};



#endif /*GUILDS_H_*/

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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
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

bool shopStaysWithGuild(const std::shared_ptr<UniqueRoom> shop);


class GuildCreation {
public:
    GuildCreation();

    bool addSupporter(std::shared_ptr<Player> supporter);
    bool removeSupporter(const std::string &supporterName);
    void renameSupporter(const std::string &oldName, const std::string &newName);
    bool saveToXml(xmlNodePtr rootNode) const;

public:
    std::string name;
    std::string leader;
    std::string leaderIp;
    int     status;
    int     numSupporters;
    std::map<std::string, std::string> supporters;
};


class Guild {
public:
    Guild();
    explicit Guild(xmlNodePtr curNode);
    void recalcLevel();
    int averageLevel();
    bool addMember(const std::string &memberName);
    bool delMember(std::string_view memberName);
    bool isMember(std::string_view memberName);
    void renameMember(std::string_view oldName, std::string_view newName);
    void parseGuildMembers(xmlNodePtr cur);
    bool saveToXml(xmlNodePtr rootNode) const;

    static void create(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void forum(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void cancel(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void support(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void invite(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void abdicate(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void abdicate(std::shared_ptr<Player> player, std::shared_ptr<Player> target, bool online=true);
    static void remove(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void join(std::shared_ptr<Player> player, cmd *cmnd);
    static void reject(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void disband(const std::shared_ptr<Player>& player, cmd* cmnd);

    static void list(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void viewMembers(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void promote(const std::shared_ptr<Player>& player, cmd* cmnd);
    static void demote(const std::shared_ptr<Player>& player, cmd* cmnd);


public:
    std::list<std::string> members;
    Money   bank;

    [[nodiscard]] std::string getName() const;
    [[nodiscard]] unsigned short getNum() const;
    [[nodiscard]] std::string getLeader() const;
    [[nodiscard]] long getLevel() const;
    [[nodiscard]] int getNumMembers() const;
    [[nodiscard]] long getPkillsIn() const;
    [[nodiscard]] long getPkillsWon() const;
    [[nodiscard]] long getPoints() const;

    void setName(std::string_view n);
    void setNum(unsigned short n);
    void setLeader(std::string_view l);
    void setLevel(long l);
    void setNumMembers(int n);
    void setPkillsIn(long pk);
    void setPkillsWon(long pk);
    void setPoints(long p);

    void incLevel(int l=1);
    void incNumMembers(int n=1);
    void incPkillsIn(long pk=1);
    void incPkillsWon(long pk=1);

    void guildhallLocations(const std::shared_ptr<Player> player, const char* fmt) const;
protected:
    std::string name;
    unsigned short num;
    std::string leader;
    long    level{};      // Sum of everyone's level in the guild
    int     numMembers{}; // Number of members in the guild
    long    pkillsIn{};
    long    pkillsWon{};
    long    points{};     // Points from quests/etc
};



#endif /*GUILDS_H_*/

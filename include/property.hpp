/*
 * property.h
 *   Header file for properties
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

#ifndef PROPERTY_H_
#define PROPERTY_H_

#include <boost/dynamic_bitset.hpp>
#include <list>
#include <map>
#include <libxml/parser.h>  // for xmlNodePtr

#include "catRef.hpp"
#include "range.hpp"

class cmd;
class AreaRoom;
class BaseRoom;
class Guild;
class MudFlag;
class Player;
class Socket;
class UniqueRoom;

enum PropLog {
    LOG_PARTIAL =   0,
    LOG_NONE =      1,
    LOG_ALL =       2
};

#define PROP_LOG_SIZE       30


class PartialOwner {
public:
    PartialOwner();
    void load(xmlNodePtr rootNode);
    void save(xmlNodePtr rootNode) const;

    void    defaultFlags(PropType type);
    [[nodiscard]] std::string getName() const;
    void    setName(std::string_view str);

    [[nodiscard]] bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);
protected:
    std::string name;
    boost::dynamic_bitset<> flags{32};
};




class Property {
public:
    Property();
    void load(xmlNodePtr rootNode);
    void save(xmlNodePtr rootNode) const;

    void found(const Player* player, PropType propType, std::string location = "", bool shouldSetArea = true);

    int     getGuild() const;
    std::string getArea() const;
    std::string getOwner() const;
    std::string getName() const;
    std::string getDateFounded() const;
    std::string getLocation() const;
    PropType getType() const;
    std::string getTypeStr() const;
    PropLog getLogType() const;
    std::string getLogTypeStr() const;

    void    setGuild(int g);
    void    setArea(std::string_view str);
    void    setOwner(std::string_view str);
    void    setName(std::string_view str);
    void    setDateFounded();
    void    setLocation(std::string_view str);
    void    setType(PropType t);
    void    setLogType(PropLog t);

    void    addRange(const CatRef& cr);
    void    addRange(const std::string &pArea, short low, short high);
    bool    isOwner(std::string_view str) const;
    bool    belongs(const CatRef& cr) const;
    void    rename(Player *player);
    void    destroy();
    bool    expelOnRemove() const;
    void    expelToExit(Player* player, bool offline);
    MudFlagMap* getFlagList();

    PartialOwner* getPartialOwner(std::string_view pOwner);
    bool    isPartialOwner(std::string_view pOwner);
    void    assignPartialOwner(std::string_view pOwner);
    void    unassignPartialOwner(std::string_view pOwner);

    int     viewLogFlag() const;
    std::string getLog() const;
    void    appendLog(std::string_view user, const char *fmt, ...);
    void    clearLog();

    std::string show(bool isOwner=false, std::string_view player="", int *i=nullptr);
    std::list<Range> ranges;
    std::list<PartialOwner> partialOwners;

    bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);

    static std::string getTypeStr(PropType propType);
    static std::string getTypeArea(PropType propType);
    static bool usePropFlags(PropType propType);
    static bool canEnter(const Player* player, const UniqueRoom* room, bool p);
    static bool goodNameDesc(const Player* player, const std::string &str, const std::string &fail, const std::string &disallow);
    static bool goodExit(const Player* player, const BaseRoom* room, const char *type, std::string_view xname);
    static bool isInside(const Player* player, const UniqueRoom* room, Property** p);
    static bool requireInside(const Player* player, const UniqueRoom* room, Property** p, PropType propType = PROP_NONE);
    static void descEdit(Socket* sock, const std::string& str);
    static void guildRoomSetup(UniqueRoom *room, const Guild* guild, bool outside);
    static void houseRoomSetup(UniqueRoom *room, const Player* player, bool outside);
    static void roomSetup(UniqueRoom *room, PropType propType, const Player* player, const Guild* guild, bool outside=false);
    static void linkRoom(BaseRoom* inside, BaseRoom* outside, const std::string&  xname);
    static UniqueRoom* makeNextRoom(UniqueRoom* r1, PropType propType, const CatRef& cr, bool exits, const Player* player, const Guild* guild, BaseRoom* room, const std::string &xname, const char *go, const char *back, bool save);
    static int rotateHouse(char *dir1, char *dir2, int rotation);
    static bool houseCanBuild(AreaRoom* aRoom, BaseRoom* room);

    static int buildFlag(int propType);

    static void manage(Player* player, cmd* cmnd, PropType propType, int x);
    static void manageDesc(Player* player, cmd* cmnd, PropType propType, int x);
    static void manageShort(Player* player, cmd* cmnd, PropType propType, int x);
    static void manageName(Player* player, cmd* cmnd, PropType propType, int x);
    static void manageFound(Player* player, cmd* cmnd, PropType propType, const Guild* guild, int x);
    static void manageExtend(Player* player, cmd* cmnd, PropType propType, Property* p, const Guild* guild, int x);
    static void manageRename(Player* player, cmd* cmnd, PropType propType, int x);
protected:
    std::string area;
    std::string owner;
    std::string name;
    std::string dateFounded;
    std::string location;
    PropType type;

    PropLog logType;
    std::list<std::string> log;

    // for guildhalls and shops, points to guild
    int     guild;
    boost::dynamic_bitset<> flags{32};
};


#endif /*PROPERTY_H_*/

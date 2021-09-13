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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#ifndef PROPERTY_H_
#define PROPERTY_H_

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
    [[nodiscard]] bstring getName() const;
    void    setName(const bstring& str);

    [[nodiscard]] bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);
protected:
    bstring name;
    char    flags[4]{};       // 32 max
};




class Property {
public:
    Property();
    void load(xmlNodePtr rootNode);
    void save(xmlNodePtr rootNode) const;

    void found(const Player* player, PropType propType, bstring location = "", bool shouldSetArea = true);

    int     getGuild() const;
    bstring getArea() const;
    bstring getOwner() const;
    bstring getName() const;
    bstring getDateFounded() const;
    bstring getLocation() const;
    PropType getType() const;
    bstring getTypeStr() const;
    PropLog getLogType() const;
    bstring getLogTypeStr() const;

    void    setGuild(int g);
    void    setArea(const bstring& str);
    void    setOwner(const bstring& str);
    void    setName(const bstring& str);
    void    setDateFounded();
    void    setLocation(const bstring& str);
    void    setType(PropType t);
    void    setLogType(PropLog t);

    void    addRange(const CatRef& cr);
    void    addRange(const bstring& area, int low, int high);
    bool    isOwner(const bstring& str) const;
    bool    belongs(const CatRef& cr) const;
    void    rename(Player *player);
    void    destroy();
    bool    expelOnRemove() const;
    void    expelToExit(Player* player, bool offline);
    MudFlagMap* getFlagList();

    PartialOwner* getPartialOwner(const bstring& pOwner);
    bool    isPartialOwner(const bstring& pOwner);
    void    assignPartialOwner(const bstring& pOwner);
    void    unassignPartialOwner(const bstring& pOwner);

    int     viewLogFlag() const;
    bstring getLog() const;
    void    appendLog(const bstring& user, const char *fmt, ...);
    void    clearLog();

    bstring show(bool isOwner=false, const bstring& player="", int *i=nullptr);
    std::list<Range> ranges;
    std::list<PartialOwner> partialOwners;

    bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);

    static bstring getTypeStr(PropType propType);
    static bstring getTypeArea(PropType propType);
    static bool usePropFlags(PropType propType);
    static bool canEnter(const Player* player, const UniqueRoom* room, bool p);
    static bool goodNameDesc(const Player* player, const bstring& str, const bstring& fail, const bstring& disallow);
    static bool goodExit(const Player* player, const BaseRoom* room, const char *type, const bstring& xname);
    static bool isInside(const Player* player, const UniqueRoom* room, Property** p);
    static bool requireInside(const Player* player, const UniqueRoom* room, Property** p, PropType propType = PROP_NONE);
    static void descEdit(Socket* sock, const bstring& str);
    static void guildRoomSetup(UniqueRoom *room, const Guild* guild, bool outside);
    static void houseRoomSetup(UniqueRoom *room, const Player* player, bool outside);
    static void roomSetup(UniqueRoom *room, PropType propType, const Player* player, const Guild* guild, bool outside=false);
    static void linkRoom(BaseRoom* inside, BaseRoom* outside, const bstring& xname);
    static UniqueRoom* makeNextRoom(UniqueRoom* r1, PropType propType, const CatRef& cr, bool exits, const Player* player, const Guild* guild, BaseRoom* room, const bstring& xname, const char *go, const char *back, bool save);
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
    bstring area;
    bstring owner;
    bstring name;
    bstring dateFounded;
    bstring location;
    PropType type;

    PropLog logType;
    std::list<bstring> log;

    // for guildhalls and shops, points to guild
    int     guild;
    char    flags[4]{};       // 32 max
};


#endif /*PROPERTY_H_*/

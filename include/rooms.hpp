/*
 * rooms.h
 *   Header file for Exit / BaseRoom / AreaRoom / Room classes
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
#ifndef ROOMS_H_
#define ROOMS_H_

// define some limits
#define NUM_RANDOM_SLOTS    50
#define NUM_PERM_SLOTS      50

class UniqueRoom;
class AreaRoom;
class Fishing;

#include "container.hpp"
#include "exits.hpp"
#include "global.hpp"
#include "enums/loadType.hpp"
#include "location.hpp"
#include "realm.hpp"
#include "size.hpp"
#include "track.hpp"
#include "wanderInfo.hpp"

typedef std::list<Exit*> ExitList;

class BaseRoom: public Container {
protected:
    void BaseDestroy();
    std::string version;    // What version of the mud this object was saved under
    bool tempNoKillDarkmetal;

public:
    //xtag  *first_ext;     // Exits
    ExitList exits;

    char    misc[64]{};       // miscellaneous space

public:
    BaseRoom();
    virtual ~BaseRoom() {};
//  virtual bool operator< (const MudObject& t) const = 0;

    void readExitsXml(xmlNodePtr curNode, bool offline=false);
    bool delExit(std::string_view dir);
    bool delExit(Exit *exit);
    void clearExits();

    [[nodiscard]] bool isSunlight() const;
    // handles darkmetal and unique
    void killMortalObjectsOnFloor();
    void killMortalObjects(bool floor=true);
    void killUniques();
    void setTempNoKillDarkmetal(bool noKillDarkmetal);
    void scatterObjects();

    [[nodiscard]] bool isCombat() const;
    [[nodiscard]] bool isConstruction() const;

    int saveExitsXml(xmlNodePtr curNode) const;


    [[nodiscard]] Monster* getGuardingExit(const Exit* exit, const Player* player) const;
    void addExit(Exit *ext);
    void checkExits();
    [[nodiscard]] bool deityRestrict(const Creature* creature) const;
    [[nodiscard]] int maxCapacity() const;
    [[nodiscard]] bool isFull() const;
    [[nodiscard]] int countVisPly() const;
    [[nodiscard]] int countCrt() const;
    [[nodiscard]] Monster* getTollkeeper() const;

    [[nodiscard]] bool isMagicDark() const;
    [[nodiscard]] bool isNormalDark() const;
    [[nodiscard]] bool isUnderwater() const;
    [[nodiscard]] bool isOutdoors() const;
    [[nodiscard]] bool isDropDestroy() const;
    [[nodiscard]] bool magicBonus() const;
    [[nodiscard]] bool isForest() const;
    [[nodiscard]] bool vampCanSleep(Socket* sock) const;
    [[nodiscard]] int getMaxMobs() const;
    [[nodiscard]] int dmInRoom() const;
    void arrangeExits(Player* player= nullptr);
    [[nodiscard]] bool isWinter() const;
    [[nodiscard]] bool isOutlawSafe() const;
    [[nodiscard]] bool isPkSafe() const;
    [[nodiscard]] bool isFastTick() const;


    [[nodiscard]] virtual bool flagIsSet(int flag) const = 0;
    virtual void setFlag(int flag) = 0;
    [[nodiscard]] virtual Size getSize() const = 0;
    [[nodiscard]] bool hasRealmBonus(Realm realm) const;
    [[nodiscard]] bool hasOppositeRealmBonus(Realm realm) const;
    WanderInfo* getWanderInfo();
    void expelPlayers(bool useTrapExit, bool expulsionMessage, bool expelStaff);

    [[nodiscard]] std::string fullName() const;
    [[nodiscard]] std::string getVersion() const;
    void setVersion(std::string_view v);
    [[nodiscard]] bool hasTraining() const;
    [[nodiscard]] CreatureClass whatTraining(int extra=0) const;

    virtual const Fishing* getFishing() const = 0;

    bool pulseEffects(time_t t);

    void addEffectsIndex();
    bool removeEffectsIndex();
    [[nodiscard]] bool needsEffectsIndex() const;


    void print(Socket* ignore, const char *fmt, ...);
    void print(Socket* ignore1, Socket* ignore2, const char *fmt, ...);

    virtual std::string getMsdp(bool showExits = true) const { return ""; };
    [[nodiscard]] std::string getExitsMsdp() const;
private:
    void doPrint(bool showTo(Socket*), Socket* ignore1, Socket* ignore2, const char *fmt, va_list ap);
};


class UniqueRoom: public BaseRoom {
public:
    UniqueRoom();
    ~UniqueRoom();
    bool operator< (const UniqueRoom& t) const;

    void escapeText();
    int readFromXml(xmlNodePtr rootNode, bool offline=false);
    int saveToXml(xmlNodePtr rootNode, int permOnly) const;
    int saveToFile(int permOnly, LoadType saveType=LoadType::LS_NORMAL);

    [[nodiscard]] std::string getShortDescription() const;
    [[nodiscard]] std::string getLongDescription() const;
    [[nodiscard]] short getLowLevel() const;
    [[nodiscard]] short getHighLevel() const;
    [[nodiscard]] short getMaxMobs() const;
    [[nodiscard]] short getTrap() const;
    [[nodiscard]] CatRef getTrapExit() const;
    [[nodiscard]] short getTrapWeight() const;
    [[nodiscard]] short getTrapStrength() const;
    [[nodiscard]] std::string getFaction() const;
    [[nodiscard]] long getBeenHere() const;
    [[nodiscard]] short getTerrain() const;
    [[nodiscard]] int getRoomExperience() const;
    [[nodiscard]] Size getSize() const;
    [[nodiscard]] bool canPortHere(const Creature* creature=0) const;

    void setShortDescription(std::string_view desc);
    void setLongDescription(std::string_view desc);
    void appendShortDescription(std::string_view desc);
    void appendLongDescription(std::string_view desc);
    void setLowLevel(short lvl);
    void setHighLevel(short lvl);
    void setMaxMobs(short m);
    void setTrap(short t);
    void setTrapExit(const CatRef& t);
    void setTrapWeight(short weight);
    void setTrapStrength(short strength);
    void setFaction(std::string_view f);
    void incBeenHere();
    void setTerrain(short t);
    void setRoomExperience(int exp);
    void setSize(Size s);

    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    std::string getMsdp(bool showExits = true) const;
protected:
    char    flags[16]{};  // Max flags - 128
    std::string fishing;

    std::string short_desc;     // Descriptions
    std::string long_desc;
    short   lowLevel;       // Lowest level allowed in
    short   highLevel;      // Highest level allowed in
    short   maxmobs;

    short   trap;
    CatRef  trapexit;
    short   trapweight;
    short   trapstrength;
    std::string faction;

    long    beenhere;       // # times room visited

    int     roomExp;
    Size    size;
public:

    CatRef  info;

    Track   track;
    WanderInfo wander;          // Random monster info
    std::map<int, crlasttime> permMonsters; // Permanent/reappearing monsters
    std::map<int, crlasttime> permObjects;  // Permanent/reappearing items

    char    last_mod[24]{};   // Last staff member to modify room.
    struct lasttime lasttime[16];   // For timed room things, like darkness spells.

    char    lastModTime[32]{}; // date created (first *saved)

    char    lastPly[32]{};
    char    lastPlyTime[32]{};

public:

    int getWeight();
    void validatePerms();
    void addPermCrt();
    void addPermObj();

    void destroy();

    bool flagIsSet(int flag) const;
    void setFlag(int flag);
    void clearFlag(int flag);
    bool toggleFlag(int flag);

    std::string getFishingStr() const;
    void setFishing(std::string_view id);
    const Fishing* getFishing() const;
};

class AreaRoom: public BaseRoom {
public:
    AreaRoom(Area *a, const MapMarker *m=0);
    ~AreaRoom();
    bool operator< (const AreaRoom& t) const;

    void reset();
    WanderInfo* getRandomWanderInfo();
    Size getSize() const;

    bool    canDelete();
    void    recycle();
    bool    updateExit(std::string_view dir);
    void    updateExits();
    bool    isInteresting(const Player *viewer) const;
    bool    isRoad() const;
    bool    isWater() const;
    bool    canSave() const;
    void    save(Player* player=0) const;
    void    load(xmlNodePtr rootNode);
    CatRef  getUnique(Creature* creature, bool skipDec=false);
    void    setMapMarker(const MapMarker* m);
    bool    spawnHerbs();

    bool flagIsSet(int flag) const;
    void setFlag(int flag);

    const Fishing* doGetFishing(short y, short x) const;
    const Fishing* getFishing() const;

    // any attempt to enter this area room (from any direction)
    // will lead you to a unique room
    CatRef      unique;

    Area        *area;
    MapMarker   mapmarker;

    bool getNeedsCompass() const;
    bool getDecCompass() const;
    bool getStayInMemory() const;

    void setNeedsCompass(bool need);
    void setDecCompass(bool dec);
    void setStayInMemory(bool stay);

    bool swap(const Swap& s);
    bool swapIsInteresting(const Swap& s) const;

    std::string getMsdp(bool showExits = true) const;
protected:
    bool    needsCompass{};
    bool    decCompass{};
    bool    stayInMemory{};
};


#endif /*ROOMS_H_*/

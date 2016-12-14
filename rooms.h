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
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
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

#include "container.h"
#include "exits.h"
#include "global.h"
#include "location.h"
#include "realm.h"
#include "size.h"
#include "track.h"
#include "wanderInfo.h"

typedef std::list<Exit*> ExitList;

class BaseRoom: public Container {
protected:
    void BaseDestroy();
    bstring version;    // What version of the mud this object was saved under
    bool tempNoKillDarkmetal;

public:
    //xtag  *first_ext;     // Exits
    ExitList exits;

    char    misc[64];       // miscellaneous space

public:
    BaseRoom();
    virtual ~BaseRoom() {};
//  virtual bool operator< (const MudObject& t) const = 0;

    void readExitsXml(xmlNodePtr curNode, bool offline=false);
    bool delExit(bstring dir);
    bool delExit(Exit *exit);
    void clearExits();

    bool isSunlight() const;
    // handles darkmetal and unique
    void killMortalObjectsOnFloor();
    void killMortalObjects(bool floor=true);
    void killUniques();
    void setTempNoKillDarkmetal(bool noKillDarkmetal);
    void scatterObjects();

    bool isCombat() const;
    bool isConstruction() const;

    int saveExitsXml(xmlNodePtr curNode) const;


    Monster* getGuardingExit(const Exit* exit, const Player* player) const;
    void addExit(Exit *ext);
    void checkExits();
    bool deityRestrict(const Creature* creature) const;
    int maxCapacity() const;
    bool isFull() const;
    int countVisPly() const;
    int countCrt() const;
    Monster* getTollkeeper() const;

    bool isMagicDark() const;
    bool isNormalDark() const;
    bool isUnderwater() const;
    bool isOutdoors() const;
    bool isDropDestroy() const;
    bool magicBonus() const;
    bool isForest() const;
    bool vampCanSleep(Socket* sock) const;
    int getMaxMobs() const;
    int dmInRoom() const;
    void arrangeExits(Player* player=0);
    bool isWinter() const;
    bool isOutlawSafe() const;
    bool isPkSafe() const;
    bool isFastTick() const;


    virtual bool flagIsSet(int flag) const = 0;
//  virtual void setFlag(int flag) = 0;
    virtual Size getSize() const = 0;
    bool hasRealmBonus(Realm realm) const;
    bool hasOppositeRealmBonus(Realm realm) const;
    WanderInfo* getWanderInfo();
    void expelPlayers(bool useTrapExit, bool expulsionMessage, bool expelStaff);

    bstring fullName() const;
    bstring getVersion() const;
    void setVersion(bstring v);
    bool hasTraining() const;
    CreatureClass whatTraining(int extra=0) const;

    virtual const Fishing* getFishing() const = 0;

    bool pulseEffects(time_t t);

    void addEffectsIndex();
    bool removeEffectsIndex();
    bool needsEffectsIndex() const;


    void print(Socket* ignore, const char *fmt, ...);
    void print(Socket* ignore1, Socket* ignore2, const char *fmt, ...);

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
    int saveToFile(int permOnly, LoadType saveType=LS_NORMAL);

    bstring getShortDescription() const;
    bstring getLongDescription() const;
    short getLowLevel() const;
    short getHighLevel() const;
    short getMaxMobs() const;
    short getTrap() const;
    CatRef getTrapExit() const;
    short getTrapWeight() const;
    short getTrapStrength() const;
    bstring getFaction() const;
    long getBeenHere() const;
    short getTerrain() const;
    int getRoomExperience() const;
    Size getSize() const;
    bool canPortHere(const Creature* creature=0) const;

    void setShortDescription(const bstring& desc);
    void setLongDescription(const bstring& desc);
    void appendShortDescription(const bstring& desc);
    void appendLongDescription(const bstring& desc);
    void setLowLevel(short lvl);
    void setHighLevel(short lvl);
    void setMaxMobs(short m);
    void setTrap(short t);
    void setTrapExit(CatRef t);
    void setTrapWeight(short weight);
    void setTrapStrength(short strength);
    void setFaction(bstring f);
    void incBeenHere();
    void setTerrain(short t);
    void setRoomExperience(int exp);
    void setSize(Size s);

    bool swap(Swap s);
    bool swapIsInteresting(Swap s) const;
protected:
    char    flags[16];  // Max flags - 128
    bstring fishing;

    bstring short_desc;     // Descriptions
    bstring long_desc;
    short   lowLevel;       // Lowest level allowed in
    short   highLevel;      // Highest level allowed in
    short   maxmobs;

    short   trap;
    CatRef  trapexit;
    short   trapweight;
    short   trapstrength;
    bstring faction;

    long    beenhere;       // # times room visited

    int     roomExp;
    Size    size;
public:

    CatRef  info;

    Track   track;
    WanderInfo wander;          // Random monster info
    std::map<int, crlasttime> permMonsters; // Permanent/reappearing monsters
    std::map<int, crlasttime> permObjects;  // Permanent/reappearing items

    char    last_mod[24];   // Last staff member to modify room.
    struct lasttime lasttime[16];   // For timed room things, like darkness spells.

    char    lastModTime[32]; // date created (first *saved)

    char    lastPly[32];
    char    lastPlyTime[32];

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

    bstring getFishingStr() const;
    void setFishing(bstring id);
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
    bool    updateExit(bstring dir);
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

    bool swap(Swap s);
    bool swapIsInteresting(Swap s) const;
protected:
    bool    needsCompass;
    bool    decCompass;
    bool    stayInMemory;
};


#endif /*ROOMS_H_*/

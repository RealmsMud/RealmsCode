/*
 * objects.h
 *   Header file for objects
 *   ____           _
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
#ifndef OBJECTS_H_
#define OBJECTS_H_

#define OBJ_KEY_LENGTH          20
#define OBJ_FLAG_ARRAY_SIZE     32

#include <list>

#include "alchemy.hpp"
#include "catRef.hpp"
#include "container.hpp"
#include "dice.hpp"
#include "global.hpp"
#include "lasttime.hpp"
#include "enums/loadType.hpp"
#include "money.hpp"
#include "range.hpp"
#include "size.hpp"

class MapMarker;

enum Material {
    NO_MATERIAL = 0,
    WOOD = 1,
    GLASS = 2,
    CLOTH = 3,
    PAPER = 4,
    IRON = 5,
    STEEL = 6,
    MITHRIL = 7,
    ADAMANTIUM = 8,
    STONE = 9,
    ORGANIC = 10,
    BONE = 11,
    LEATHER = 12,
    DARKMETAL = 13,
    CRYSTAL = 14,

    MAX_MATERIAL
};

enum class ObjectType {
    // 0,1,2 open

    INSTRUMENT = 3, // Bard Instrument

    HERB = 4,   // Item is a herb!  Subtype if needed in subtype

    WEAPON = 5,

    ARMOR = 6,
    POTION = 7,
    SCROLL = 8,
    WAND = 9,
    CONTAINER = 10,

    MONEY = 11,
    KEY = 12,
    LIGHTSOURCE = 13,
    MISC = 14,
    SONGSCROLL = 15,
    POISON = 16,
    BANDAGE = 17,
    AMMO = 18,
    QUIVER = 19,
    LOTTERYTICKET = 20,

    MAX_OBJECT_TYPE,
};

#include "objIncrease.hpp"

class DroppedBy {
    friend class Object;
    friend std::ostream& operator<<(std::ostream& out, const DroppedBy& drop);
protected:
    bstring name;
    bstring index;
    bstring id;
    bstring type;
public:
    DroppedBy& operator=(const DroppedBy& o);

    bstring str();
    bstring getName() const;
    bstring getIndex() const;
    bstring getId() const;
    bstring getType() const;


    void clear();
    void save(xmlNodePtr rootNode) const;
    void load(xmlNodePtr rootNode);
};

typedef std::map<int, AlchemyEffect> AlchemyEffectMap;

class Object: public Container, public Containable {
    // Static class functions
public:
    static Object* getNewPotion();  // Creates a new blank potion object
    static const std::map<ObjectType,const char*> objTypeToString;

public:
    Object();
    ~Object();
    Object& operator=(const Object& o);
    bstring getCompareStr() const ;
    bool operator==(const Object& o) const;
    bool operator!=(const Object& o) const;
    bool operator< (const Object& t) const;
    void validateId();

protected:
    short           weight;
    short           bulk;           // item bulk rating.
    short           maxbulk;
    Size            size;
    ObjectType      type;
    short           wearflag;       // Where/if it can be worn
    short           armor;          // AC adjustment
    short           quality;        // Quality of the item (1-400) for auto adjustment of AC
    short           adjustment;
    short           numAttacks;     // Number of attacks this weapon offers (IE: cat'o nine tails)
    short           shotsMax;       // Shots before breakage
    short           shotsCur;
    short           chargesMax;     // Maximum charges (Currently only used for casting weapons)
    short           chargesCur;
    short           magicpower;
    short           level;
    int             requiredSkill;  // Required amount of skill gain to use this object
    short           minStrength;
    short           clan;
    short           special;
    short           questnum;       // Quest fulfillment number
    bstring         effect;
    long            effectDuration;
    short           effectStrength;
    unsigned long   coinCost;
    unsigned long   shopValue;
    long            made;           // the time this object was created
    short           keyVal;         // key # value to match exit
    int             lotteryCycle;
    short           lotteryNumbers[6]{};
    int             recipe;
    Material        material;
    // SubType: For Armor - The type of armor it is, ie: cloth, leather, chain, plate, etc
    //          For Weapons - The weapon class it is, sword, dagger, etc
    //          For Alchemy - The type of device it is, mortar and pestle, etc
    bstring subType;
    char flags[OBJ_FLAG_ARRAY_SIZE]{};  // Max object flags - 256
    short delay;
    short extra;
    bstring questOwner;

protected:
    void doCopy(const Object& o);
    void selectRandom();  // become a random object

// Public member variables
public:
    CatRef info;
    ObjIncrease* increase;

    // Strings
    bstring description;
    bstring version;    // What version of the mud this object was saved under
    bstring lastMod;    // Last staff member to modify object.

    // Information about where this object came from
    DroppedBy droppedBy;

    char key[3][OBJ_KEY_LENGTH]{};    // key 3 hold object index
    char use_output[80]{};    // String output by successful use
    char use_attack[50]{};

    Dice damage;
    double getDps();


    CatRef in_bag[3];   // items preloaded inside bags

    struct lasttime lasttime[4];// Last-time-used variables - Used for envenom and enchant right now

    Range deed;
    Money value;        // coin value

    // where we store refund information - this never needs to be loaded
    // or saved from file because if they log, they can't refund!
    Money refund;
    MapMarker *compass; // for compass objects
    bstring plural;

    // List of effects that are conferred by this item.  Name, duration, and strength.
    //  std::list<ConferredEffect*> conferredEffects;

    bool pulseEffects(long t) { return (false); };

    std::list<CatRef> randomObjects;

    bool isAlchemyPotion();
    bool consumeAlchemyPotion(Creature* consumer);
    bool addAlchemyEffect(int num, const AlchemyEffect &ae);

    // Map of effects that are on this item for alchemy purposes, used for herbs and potions objects
    AlchemyEffectMap alchemyEffects;

// Functions
public:

    void init(bool selRandom = true);
    // Xml - Loading
    int readFromXml(xmlNodePtr rootNode, std::list<bstring> *idList = 0, bool offline=false);
    void loadAlchemyEffects(xmlNodePtr curNode);

    // Xml - Saving
    int saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType = LoadType::LS_FULL, int quantity = 1,
                  bool saveId = true, std::list<bstring> *idList = 0) const;
    int saveToFile();

    void setDroppedBy(MudObject* dropper, const bstring& pDropType);

    bool isBroken() const;

    // Flags
    bool flagIsSet(int flag) const; // *
    void setFlag(int flag); // *
    void clearFlag(int flag); // *
    bool toggleFlag(int flag); // *

//    char* cmpName();
    void escapeText();

    bstring getFlagList(bstring sep=", ") const;

    // Placement of the object etc
    void addObj(Object *toAdd, bool incShots = true); // Add an object to this object
    void delObj(Object  *toDel);
    void addToRoom(BaseRoom* room);
    void deleteFromRoom();
    void popBag(Creature* creature, bool quest = true, bool drop = true, bool steal = true,
                bool bodypart = true, bool dissolve = false);
    int doSpecial(Player* player);
    int countObj(bool permOnly = false);

    // Get
    [[nodiscard]] short getDelay() const;
    [[nodiscard]] short getExtra() const;
    [[nodiscard]] short getWeight() const;
    [[nodiscard]] int getActualWeight() const;
    [[nodiscard]] short getBulk() const;
    [[nodiscard]] int getActualBulk() const;
    [[nodiscard]] short getMaxbulk() const;
    [[nodiscard]] short getWeaponDelay() const;
    [[nodiscard]] float getLocationModifier() const;
    [[nodiscard]] float getTypeModifier() const;
    [[nodiscard]] unsigned short getKey() const;
    [[nodiscard]] Size getSize() const;
    [[nodiscard]] bstring getSizeStr() const;
    [[nodiscard]] ObjectType getType() const; // *
    [[nodiscard]] bstring getTypeName() const;
    [[nodiscard]] short getWearflag() const; // *
    [[nodiscard]] short getArmor() const;
    [[nodiscard]] short getQuality() const;
    [[nodiscard]] short getAdjustment() const;
    [[nodiscard]] short getNumAttacks() const;
    [[nodiscard]] short getShotsMax() const; // *
    [[nodiscard]] short getShotsCur() const; // *
    [[nodiscard]] short getChargesMax() const;
    [[nodiscard]] short getChargesCur() const;
    [[nodiscard]] short getMagicpower() const;
    [[nodiscard]] short getLevel() const;
    [[nodiscard]] int getRequiredSkill() const;
    [[nodiscard]] short getMinStrength() const;
    [[nodiscard]] short getClan() const;
    [[nodiscard]] short getSpecial() const;
    [[nodiscard]] short getQuestnum() const;
    [[nodiscard]] bstring getEffect() const;
    [[nodiscard]] long getEffectDuration() const;
    [[nodiscard]] short getEffectStrength() const;
    [[nodiscard]] unsigned long getCoinCost() const;
    [[nodiscard]] unsigned long getShopValue() const;
    [[nodiscard]] long getMade() const;
    [[nodiscard]] int getLotteryCycle() const;
    [[nodiscard]] short getLotteryNumbers(short i) const;
    [[nodiscard]] int getRecipe() const;
    [[nodiscard]] Material getMaterial() const;
    [[nodiscard]] bstring getMaterialName() const;
    [[nodiscard]] bstring getCompass(const Creature* creature, bool useName);
    [[nodiscard]] bstring getVersion() const;
    [[nodiscard]] bstring getQuestOwner() const;
    [[nodiscard]] bstring getObjStr(const Creature* viewer = nullptr, unsigned int ioFlags = 0, int num = 0) const;
    [[nodiscard]] bstring getSubType() const;
    [[nodiscard]] bstring getWeaponType() const;
    [[nodiscard]] bstring getArmorType() const;
    [[nodiscard]] bstring getWeaponCategory() const;
    [[nodiscard]] bstring getWeaponVerb() const;
    [[nodiscard]] bstring getWeaponVerbPlural() const;
    [[nodiscard]] bstring getWeaponVerbPast() const;

    bool isQuestOwner(const Player* player) const;
    bstring getWearName();


    void nameAlchemyPotion(bool potion = true);

    // Set
    void setKey(unsigned short k);
    bool setWeaponType(const bstring& newType);
    bool setArmorType(const bstring& newType);
    bool setSubType(const bstring& newType);
    void setMade();
    void setDelay(int newDelay);
    void setExtra(int x);
    void setWeight(short w);
    void setBulk(short b);
    void setMaxbulk(short b);
    void setSize(Size s);
    void setType(ObjectType t);
    void setWearflag(short w);
    void setArmor(short a);
    void setQuality(short q);
    void setAdjustment(short a);
    void setNumAttacks(short n);
    void setShotsMax(short s);
    void setShotsCur(short s);
    void decShotsCur(short s = 1);
    void incShotsCur(short s = 1);
    void decChargesCur(short s = 1);
    void incChargesCur(short s = 1);
    void setChargesMax(short s);
    void setChargesCur(short s);
    void setMagicpower(short m);
    void setMagicrealm(short m);
    void setLevel(short l);
    void setRequiredSkill(int s);
    void setMinStrength(short s);
    void setClan(short c);
    void setSpecial(short s);
    void setQuestnum(short q);
    void setEffect(const bstring& e);
    void setEffectDuration(long d);
    void setEffectStrength(short s);
    void setCoinCost(unsigned long c);
    void setShopValue(unsigned long v);
    void setLotteryCycle(int c);
    void setLotteryNumbers(short i, short n);
    void setRecipe(int r);
    void setMaterial(Material m);
    void setQuestOwner(const Player* player);
    void setUniqueId(int id);

    void clearEffect();

    // Adjust things
    void killUniques();
    void loadContainerContents();
    void nameCoin(const bstring& type, unsigned long value);
    void randomEnchant(int bonus = 0);
    int adjustArmor();
    int adjustWeapon();
    void tempPerm();
    void track(Player* player);

    // Check conditions
    [[nodiscard]] bool doRestrict(Creature* creature, bool p);
    [[nodiscard]] bool raceRestrict(const Creature* creature) const;
    [[nodiscard]] bool raceRestrict(const Creature* creature, bool p) const;
    [[nodiscard]] bool classRestrict(const Creature* creature) const;
    [[nodiscard]] bool classRestrict(const Creature* creature, bool p) const;
    [[nodiscard]] bool clanRestrict(const Creature* creature) const;
    [[nodiscard]] bool clanRestrict(const Creature* creature, bool p) const;
    [[nodiscard]] bool levelRestrict(const Creature* creature, bool p = false) const;
    [[nodiscard]] bool skillRestrict(const Creature* creature, bool p = false) const;
    [[nodiscard]] bool alignRestrict(const Creature* creature, bool p = false) const;
    [[nodiscard]] bool sexRestrict(const Creature* creature, bool p = false) const;
    [[nodiscard]] bool strRestrict(Creature* creature, bool p = false) const;
    [[nodiscard]] bool lawchaoRestrict(const Creature* creature, bool p = false) const;
    [[nodiscard]] bool showAsSame(const Player* player, const Object* Object) const;
    [[nodiscard]] bool isHeavyArmor() const;
    [[nodiscard]] bool isMediumArmor() const;
    [[nodiscard]] bool isLightArmor() const;
    [[nodiscard]] bool needsTwoHands() const;
    [[nodiscard]] bool isQuestValid() const; // Is this object valid for a quest?

    bstring showAlchemyEffects(Player *player = nullptr);
    bstring statObj(unsigned int statFlags);
    double winterProtection() const;
    bool isKey(const UniqueRoom* room, const Exit* exit) const;

    bool swap(Swap s);
    bool swapIsInteresting(Swap s) const;

};


#endif /*OBJECTS_H_*/

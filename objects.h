/*
 * objects.h
 *	 Header file for objects
 *   ____			_
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *	http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef OBJECTS_H_
#define OBJECTS_H_

#define OBJ_KEY_LENGTH			20
#define OBJ_FLAG_ARRAY_SIZE		32

#include "alchemy.h"

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

#include "objIncrease.h"

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

class Object: public MudObject {
    // Static class functions
public:
    static Object* getNewPotion();	// Creates a new blank potion object

public:
    Object();
    ~Object();
    Object& operator=(const Object& o);
    bool operator==(const Object& o) const;
    bool operator!=(const Object& o) const;
    bool operator< (const MudObject& t) const;
    void validateId();

protected:
    short           weight;
    short           bulk;		    // item bulk rating.
    short           maxbulk;
    Size            size;
    short           type;
    short           wearflag;	    // Where/if it can be worn
    short           armor;		    // AC adjustment
    short           quality;	    // Quality of the item (1-400) for auto adjustment of AC
    short           adjustment;
    short           numAttacks;     // Number of attacks this weapon offers (IE: cat'o nine tails)
    short           shotsMax;	    // Shots before breakage
    short           shotsCur;
    short           chargesMax;     // Maximum charges (Currently only used for casting weapons)
    short           chargesCur;
    short           magicpower;
    short           level;
    int             requiredSkill;  // Required amount of skill gain to use this object
    short           minStrength;
    short           clan;
    short           special;
    short           questnum;		// Quest fulfillment number
    bstring         effect;
    long            effectDuration;
    short           effectStrength;
    unsigned long   coinCost;
    unsigned long   shopValue;
    long            made;		    // the time this object was created
    short           keyVal;			// key # value to match exit
    int             lotteryCycle;
    short           lotteryNumbers[6];
    int             recipe;
    Material        material;
    // SubType:	For Armor - The type of armor it is, ie: cloth, leather, chain, plate, etc
    //			For Weapons - The weapon class it is, sword, dagger, etc
    //			For Alchemy - The type of device it is, mortar and pestle, etc
    bstring subType;
    char flags[OBJ_FLAG_ARRAY_SIZE];  // Max object flags - 256
    short delay;
    short extra;
    bstring questOwner;
    int uniqueId;

protected:
    void doCopy(const Object& o);
    void selectRandom();  // become a random object

// Public member variables
public:
    CatRef info;
    ObjIncrease* increase;

    // Strings
    bstring description;
    bstring version;	// What version of the mud this object was saved under
    bstring lastMod;	// Last staff member to modify object.

    // Information about where this object came from
    DroppedBy droppedBy;

    char key[3][OBJ_KEY_LENGTH];	// key 3 hold object index
    char use_output[80];	// String output by successful use
    char use_attack[50];

    Dice damage;
    double getDps();

    otag *first_obj;	// objects contained inside
    class Object *parent_obj;	// object this is in
    class BaseRoom *parent_room;	// room this is in
    class Creature *parent_crt;	// creature this is in

    CatRef in_bag[3];	// items preloaded inside bags

    struct lasttime lasttime[4];// Last-time-used variables - Used for envenom and enchant right now

    Range deed;
    Money value;	   	// coin value

    // where we store refund information - this never needs to be loaded
    // or saved from file because if they log, they can't refund!
    Money refund;
    MapMarker *compass;	// for compass objects
    bstring plural;

    // List of effects that are conferred by this item.  Name, duration, and strength.
    //	std::list<ConferredEffect*> conferredEffects;

    bool pulseEffects(long t) { return (false); };

    std::list<CatRef> randomObjects;

    bool isAlchemyPotion();
    bool consumeAlchemyPotion(Creature* consumer);
    bool addAlchemyEffect(int num, const AlchemyEffect &ae);

    // Map of effects that are on this item for alchemy purposes, used for herbs and potions objects
    std::map<int, AlchemyEffect> alchemyEffects;

// Functions
public:

    void init(bool selRandom = true);
    // Xml - Loading
    int readFromXml(xmlNodePtr rootNode);
    void loadAlchemyEffects(xmlNodePtr curNode);

    // Xml - Saving
    int saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType = LS_FULL, int quantity = 1,
                  bool saveId = true) const;
    int saveToFile();

    void setDroppedBy(MudObject* dropper, bstring pDropType);

    bool isBroken() const;

    // Flags
    bool flagIsSet(int flag) const; // *
    void setFlag(int flag); // *
    void clearFlag(int flag); // *
    bool toggleFlag(int flag); // *

    char* cmpName();
    void escapeText();

    // Placement of the object etc
    void addObj(Object *toAdd); // Add an object to this object
    void addToRoom(BaseRoom* room);
    void deleteFromRoom();
    void popBag(Creature* creature, bool quest = true, bool drop = true, bool steal = true,
                bool bodypart = true, bool dissolve = false);
    int doSpecial(Player* player);

    // Get
    short getDelay() const;
    short getExtra() const;
    short getWeight() const;
    int getActualWeight() const;
    short getBulk() const;
    int getActualBulk() const;
    short getMaxbulk() const;
    short getWeaponDelay() const;
    float getLocationModifier() const;
    float getTypeModifier() const;
    unsigned short getKey() const;
    Size getSize() const;
    short getType() const; // *
    short getWearflag() const; // *
    short getArmor() const;
    short getQuality() const;
    short getAdjustment() const;
    short getNumAttacks() const;
    short getShotsMax() const; // *
    short getShotsCur() const; // *
    short getChargesMax() const;
    short getChargesCur() const;
    short getMagicpower() const;
    short getLevel() const;
    int getRequiredSkill() const;
    short getMinStrength() const;
    short getClan() const;
    short getSpecial() const;
    short getQuestnum() const;
    bstring getEffect() const;
    long getEffectDuration() const;
    short getEffectStrength() const;
    unsigned long getCoinCost() const;
    unsigned long getShopValue() const;
    long getMade() const;
    int getLotteryCycle() const;
    short getLotteryNumbers(short i) const;
    int getRecipe() const;
    Material getMaterial() const;
    bstring getCompass(const Creature* creature, bool useName);
    bstring getVersion() const;
    bstring getQuestOwner() const;
    bstring getObjStr(const Creature* viewer = NULL, int flags = 0, int num = 0) const;
    const bstring getSubType() const;
    const bstring getWeaponType() const;
    const bstring getArmorType() const;
    const bstring getWeaponCategory() const;
    const bstring getWeaponVerb() const;
    const bstring getWeaponVerbPlural() const;
    const bstring getWeaponVerbPast() const;

    bool isQuestOwner(const Player* player) const;
    bstring getWearName();
    int getUniqueId() const;

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
    void setType(short t);
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
    void setEffect(bstring e);
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
    void nameCoin(bstring type, unsigned long value);
    void randomEnchant(int bonus = 0);
    int adjustArmor();
    int adjustWeapon();
    void tempPerm();
    void track(Player* player);

    // Check conditions
    bool doRestrict(Creature* creature, bool p);
    bool raceRestrict(const Creature* creature) const;
    bool raceRestrict(const Creature* creature, bool p) const;
    bool classRestrict(const Creature* creature) const;
    bool classRestrict(const Creature* creature, bool p) const;
    bool clanRestrict(const Creature* creature) const;
    bool clanRestrict(const Creature* creature, bool p) const;
    bool levelRestrict(const Creature* creature, bool p = false) const;
    bool skillRestrict(const Creature* creature, bool p = false) const;
    bool alignRestrict(const Creature* creature, bool p = false) const;
    bool sexRestrict(const Creature* creature, bool p = false) const;
    bool strRestrict(const Creature* creature, bool p = false) const;
    bool lawchaoRestrict(const Creature* creature, bool p = false) const;
    bool showAsSame(const Player* player, const Object* Object) const;
    bool isHeavyArmor() const;
    bool isMediumArmor() const;
    bool isLightArmor() const;
    bool needsTwoHands() const;
    bool isQuestValid() const; // Is this object valid for a quest?

    bstring showAlchemyEffects(Creature *creature = NULL);
    bstring statObj(int statFlags);
    double winterProtection() const;
    bool isKey(const UniqueRoom* room, const Exit* exit) const;

    bool swap(Swap s);
    bool swapIsInteresting(Swap s) const;
};


#endif /*OBJECTS_H_*/

/*
 * creatures.hpp
 *   Header file for creature / monster / player classes
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

#include <boost/dynamic_bitset.hpp>
#include <map>
#include <string>
#include <fmt/format.h>

#include "mudObjects/container.hpp"
#include "mudObjects/mudObject.hpp"
#include "carry.hpp"
#include "creatureStreams.hpp"
#include "damage.hpp"
#include "enums/loadType.hpp"
#include "fighters.hpp"
#include "global.hpp"
#include "group.hpp"
#include "lasttime.hpp"
#include "location.hpp"
#include "magic.hpp"
#include "monType.hpp"
#include "quests.hpp"
#include "range.hpp"
#include "realm.hpp"
#include "skills.hpp"
#include "specials.hpp"                        // for SpecialAttack
#include "statistics.hpp"
#include "structs.hpp"
#include "threat.hpp"

class cmd;
class Object;
class Skill;
class Socket;
class Song;


enum AttackType {
    ATTACK_NORMAL,
    ATTACK_BACKSTAB,
    ATTACK_AMBUSH,
    ATTACK_BASH,
    ATTACK_MAUL,
    ATTACK_KICK,

    ATTACK_TYPE_MAX
};
enum AttackResult {
    ATTACK_MISS,
    ATTACK_DODGE,
    ATTACK_PARRY,
    ATTACK_RIPOSTE,
    ATTACK_GLANCING,
    ATTACK_BLOCK,
    ATTACK_CRITICAL,
    ATTACK_FUMBLE,
    ATTACK_HIT,

    ATTACK_MAX
};

class cDay;
class StartLoc;
class Monster;
class Recipe;
class TalkResponse;
class QuestInfo;
class QuestCompletion;
class QuestCompleted;
class QuestCatRef;
class SpellData;

// NOTE: It is important to use these defines for loading in an array of aray of *chars*
// ie: attack[3][30].  The size in the structure and the size passed to the loadStringArray
// function must be the same, this ensures it.  If creating any more arrays of this type use
// a similar define and use it in files-xml-read.c as well.
// -- Bane
#define CRT_ATTACK_LENGTH       30
#define CRT_KEY_LENGTH          20
#define CRT_MOVETYPE_LENGTH     52

#define NUM_ASSIST_MOB          8
#define NUM_ENEMY_MOB           4

#define DEL_ROOM_DESTROYED      1
#define DEL_PORTAL_DESTROYED    2
#define NUM_RESCUE              2

class SkillGain;


enum DamageType {
    DMG_NONE = 0,
    PHYSICAL_DMG = 1,
    MENTAL_DMG = 2,
    MAGICAL_DMG = 3
};

enum DeathCheck {
    NO_CHECK,
    CHECK_DIE,
    CHECK_DIE_ROB,
    MAX_CHECK
};

#include "timer.hpp"


class CustomCrt {
public:
    CustomCrt();

    short   community;

    short   parents;
    short   brothers;
    short   sisters;

    short   social;
    short   education;
    short   height;
    short   weight;

    std::string hair;
    std::string eyes;
    std::string skin;


    static const int PARENTS_UNKNOWN = -1;
    static const int PARENTS_DEAD = 0;
    static const int PARENTS_MOTHER_DEAD = 1;
    static const int PARENTS_FATHER_DEAD = 2;
    static const int PARENTS_ALIVE = 3;

    static const int COMMUNITY_OUTCAST = 1;
    static const int COMMUNITY_HAMLET = 2;
    static const int COMMUNITY_VILLAGE = 3;
    static const int COMMUNITY_LARGE_VILLAGE = 4;
    static const int COMMUNITY_CITY = 5;

    static const int SOCIAL_OUTCAST = 1;
    static const int SOCIAL_CRIMINAL = 2;
    static const int SOCIAL_POOR = 3;
    static const int SOCIAL_LOWER = 4;
    static const int SOCIAL_MIDDLE = 5;
    static const int SOCIAL_UPPER = 6;
    static const int SOCIAL_NOBLE = 7;

    static const int EDUCATION_NONE = 1;
    static const int EDUCATION_APPRENTICE = 2;
    static const int EDUCATION_BASIC = 3;
    static const int EDUCATION_NORMAL = 4;
    static const int EDUCATION_UNIVERSITY = 5;
};

typedef std::list<std::shared_ptr<Monster> > PetList;
typedef std::map<std::string, Skill*> SkillMap;
//*********************************************************************
//                      Creature
//*********************************************************************

class Creature: public virtual MudObject, public Streamable, public Container, public Containable {

protected:
    void crtReset();

protected:
// Data
    unsigned short level{};
    CreatureClass cClass;
    unsigned short race{};
    short alignment{};
    mType type; // creature type
    int attackPower{}; // Attack power increases base damage of a creature
    int armor{};
    unsigned long experience{};
    unsigned long temp_experience{}; // Temp experience.
    unsigned short deity{};
    Size size;
    unsigned short clan{};
    unsigned short poison_dur{};
    unsigned short poison_dmg{};
    std::string description;
    std::string version; // Version of the mud this creature was saved under
    boost::dynamic_bitset<> flags{256};
    unsigned long realm[MAX_REALM-1]{}; // Magic Spell realms
    boost::dynamic_bitset<> spells{256};
    boost::dynamic_bitset<> old_quests{256};
    static const short OFFGUARD_REMOVE;
    static const short OFFGUARD_NOREMOVE;
    static const short OFFGUARD_NOPRINT;
    DeathType deathtype;
    std::string poisonedBy;     // displayed to player, if this is a player, they get credit for pkill

    Group* group{};
    GroupStatus groupStatus;

public:
// Constructors, Deconstructors, etc
    Creature();
    Creature(Creature& cr);
    Creature(const Creature& cr);
    void crtCopy(const Creature& cr, bool assign=false);
    virtual ~Creature();



    virtual void upgradeStats() {};
    virtual std::shared_ptr<Socket> getSock() const;
    Location getLocation();
    void delayedAction(const std::string& action, int delay, const std::shared_ptr<MudObject>& target=nullptr);
    void delayedScript(const std::string& script, int delay);

    std::shared_ptr<Creature> getMaster();
    std::shared_ptr<const Creature> getConstMaster() const;

    std::shared_ptr<Player> getPlayerMaster();
    std::shared_ptr<const Player> getConstPlayerMaster() const;


    bool isPlayer() const override;
    bool isMonster() const override;
    virtual bool hasSock() const;
    bool checkMp(int reqMp);
    bool checkResource(ResourceType resType, int resCost);
    void subResource(ResourceType resType, int resCost);
    void subMp(int reqMp);

    std::weak_ptr<Creature> myTarget{};
    bool hasTarget{};
    PetList pets;
    std::list<std::weak_ptr<Creature>> targetingThis;

    MagicType getCastingType() const;
    int doHeal(const std::shared_ptr<Creature>& target, int amt, double threatFactor = 0.5);

    std::string doReplace(std::string fmt, const std::shared_ptr<MudObject>& actor=nullptr, const std::shared_ptr<MudObject>& applier=nullptr) const;

    void unApplyTongues();
    void unSilence();
    void unBlind();
    void stand();

    bool inSameRoom(const std::shared_ptr<Creature>& target);


    std::shared_ptr<Creature> findVictim(cmd* cmnd, int cmndNo, bool aggressive=true, bool selfOk=false, const std::string &noVictim="", const std::string &notFound="");
    std::shared_ptr<Creature> findVictim(const std::string &toFind, int num, bool aggressive=true, bool selfOk=false, const std::string &noVictim="", const std::string &notFound="");
    std::shared_ptr<Creature> findMagicVictim(const std::string &toFind, int num, SpellData* spellData, bool aggressive=true, bool selfOk=false, const std::string &noVictim="", const std::string &notFound="");

    bool hasAttackableTarget();
    bool isAttackingTarget();
    std::shared_ptr<Creature> getTarget();
    std::shared_ptr<Creature> addTarget(const std::shared_ptr<Creature>& toTarget);
    void checkTarget(const std::shared_ptr<Creature>& toTarget);
    void addTargetingThis(const std::shared_ptr<Creature>& targeter);
    void clearTarget(bool clearTargetsList = true);
    void clearTargetingThis(Creature *targeter);

    long getLTLeft(int myLT, long t = -1); // gets the time left on a LT
    void setLastTime(int myLT, long t, long interval); // Sets a LT

    virtual std::string getClassString() const;

public:
// Data
    std::string plural;
    std::map<std::string, long> factions;
    SkillMap skills;
    char key[3][CRT_KEY_LENGTH]{};
    int fd{}; // Socket number
    short current_language{};
    long proficiency[6]{}; // Weapon proficiencies: 6 now..added cleaving weapons
    int afterProf{};
    Dice damage;
    Money coins;
    //CatRef room;
    short questnum{}; // Quest fulfillment number (M)
    std::vector<std::shared_ptr<Object>> ready;// Worn/readied items
    //etag *first_enm; // List of enemies
    ttag *first_tlk{}; // List of talk responses

    struct saves saves[6]; // Saving throws struct. POI, DEA, BRE, MEN, SPL, x, x
    boost::dynamic_bitset<> languages{128};
    char movetype[3][CRT_MOVETYPE_LENGTH]{}; // Movement types..."flew..oozed...etc.."
    Stat strength;
    Stat dexterity;
    Stat constitution;
    Stat intelligence;
    Stat piety;
    Stat hp;
    Stat mp;
    Stat pp; // psi points for psionicists
    Stat rp; // recovery points for weaponless classes
    struct daily daily[20]; // added more daily limits
    LastTime lasttime[128]; // added more timers

    // future new tic amount code
    struct tic hpTic;
    struct tic mpTic;
    struct tic ppTic;
    struct tic rpTic;

    struct spellTimer spellTimer[16]; // spell effect timers (specific to magic)
    char misc[21]{}; // miscellaneous space

    //EffectList effects; // List of all effects on this creature
    Timer attackTimer;
    std::list<SpecialAttack> specials; // List of all special attack this creature has
    std::list<std::string> minions; // vampire minions


    Location currentLocation;
    Location previousRoom; // last room they were in
    void setPreviousRoom();

// Groups & Pets
    void setGroup(Group* newGroup);
    void setGroupStatus(GroupStatus newStatus);

    Group* getGroup(bool inGroup = true);
    GroupStatus getGroupStatus();
    std::shared_ptr<Creature> getGroupLeader();

    void addToGroup(Group* toJoin, bool announce = true);
    void createGroup(const std::shared_ptr<Creature>& crt);
    bool removeFromGroup(bool announce = true);

    bool inSameGroup(const std::shared_ptr<Creature>& target);

    void dismissPet(const std::shared_ptr<Monster>&  toDismiss);
    void dismissAll();
    void displayPets();
    void addPet(std::shared_ptr<Monster>  newPet, bool setPetFlag = true);
    void delPet(const std::shared_ptr<Monster>&  toDel);
    bool hasPet() const;

    std::shared_ptr<Monster>  findPet(const std::shared_ptr<Monster>&  toFind);
    std::shared_ptr<Monster>  findPet(const std::string& pName, int pNum);

// XML: loading and saving
    int saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType, bool saveID = true) const;
    void savePets(xmlNodePtr curNode) const;
    void saveSkills(xmlNodePtr rootNode) const;
    void saveFactions(xmlNodePtr rootNode) const;
    void saveAttacks(xmlNodePtr rootNode) const;
    int readFromXml(xmlNodePtr rootNode, bool offline=false);
    void loadAttacks(xmlNodePtr rootNode);
    void loadFactions(xmlNodePtr rootNode);
    bool loadFaction(xmlNodePtr rootNode);
    void loadSkills(xmlNodePtr rootNode);
    void loadStats(xmlNodePtr curNode);

    bool pulseEffects(time_t t) override;

    bool convertFlag(int flag);
    void removeStatEffects();
    bool doPetrificationDmg();
    bool willIgnoreIllusion() const;
    bool isInvisible() const; // *

// Skills
    bool knowsSkill(const std::string& skillName) const; // *
    double getSkillLevel(const std::string& skillName, bool useBase = true) const; // *
    double getSkillGained(const std::string& skillName, bool useBase = true) const; // *
    double getTradeSkillGained(const std::string& skillName, bool useBase = true) const; // *
    Skill* getSkill(const std::string& skillName, bool useBase = true) const;
    void addSkill(const std::string& skillName, int gained); // *
    void remSkill(const std::string& skillName); // *
    void checkSkillsGain(const std::list<SkillGain*>::const_iterator& begin, const std::list<SkillGain*>::const_iterator& end, bool setToLevel = false);
    void checkImprove(const std::string& skillName, bool success, int attribute = INT, int bns = 0); // *
    bool setSkill(const std::string& skill, int gained); // *

    // Movement


// Formatting
    virtual void escapeText() {};
    std::string getCrtStr(const std::shared_ptr<const Creature> & viewer = nullptr, int ioFlags = 0, int num = 0) const;
    std::string statCrt(int statFlags);
    int displayFlags() const;
    std::string alignColor() const;
    std::string alignString() const;
    std::string fullName() const;
    const char *hisHer() const;
    const char *himHer() const;
    const char *heShe() const;
    const char *upHisHer() const;
    const char *upHeShe() const;
    void pleaseWait(long duration) const;
    void pleaseWait(int duration) const;
    void pleaseWait(double duration) const;
    const char* getStatusStr(int dmg=0);
    virtual std::string customColorize(const std::string& text, bool caret=true) const = 0;

    void printPaged(std::string_view toPrint) const;
    void bPrint(std::string_view toPrint) const;
    void bPrintPython(const std::string& toPrint) const;

    void print(const char *fmt, ...) const;
    void printColor(const char *fmt, ...) const;

    virtual void vprint(const char *fmt, va_list ap) const {};

// Combat & Death
    std::shared_ptr<Creature>findFirstEnemyCrt(const std::shared_ptr<Creature>&pet);
    bool checkDie(const std::shared_ptr<Creature> &killer); // *
    bool checkDie(const std::shared_ptr<Creature> &killer, bool &freeTarget); // *
    int checkDieRobJail(const std::shared_ptr<Monster>& killer); // *
    int checkDieRobJail(const std::shared_ptr<Monster>& killer, bool &freeTarget); // *
    void checkDoctorKill(const std::shared_ptr<Creature>&victim);
    void die(std::shared_ptr<Creature>killer); // *
    void die(const std::shared_ptr<Creature>&killer, bool &freeTarget); // *
    void clearAsPetEnemy();
    virtual void gainExperience(const std::shared_ptr<Monster> &victim, const std::shared_ptr<Creature> &killer, int expAmount, bool groupExp = false) {} ;
    void adjustExperience(const std::shared_ptr<Monster>&  victim, int& expAmount, int& holidayExp, int& bonusExp);
    int doWeaponResist(int dmg, const std::string &weaponCategory) const;
    int doDamage(std::shared_ptr<Creature> target, int dmg, DeathCheck shouldCheckDie = CHECK_DIE, DamageType dmgType = PHYSICAL_DMG);
    int doDamage(const std::shared_ptr<Creature>& target, int dmg, DeathCheck shouldCheckDie, DamageType dmgType, bool &freeTarget);
    bool chkSave(short savetype, const std::shared_ptr<Creature>& target, short bns);
    int castWeapon(const std::shared_ptr<Creature>& target, std::shared_ptr<Object>  weapon, bool &meKilled);
    void castDelay(long delay);
    void attackDelay(long delay);
    void stun(int delay);
    bool doLagProtect();
    bool hasCharm(const std::string &charmed);
    bool inCombat(bool countPets) const;
    bool inCombat(const std::shared_ptr<Creature> & target=nullptr, bool countPets=false) const;
    bool canAttack(const std::shared_ptr<Creature>& target, bool stealing=false);
    int checkRealmResist(int dmg, Realm pRealm) const;
    void knockUnconscious(long duration);
    void clearAsEnemy();
    bool checkAttackTimer(bool displayFail = true) const;
    void updateAttackTimer(bool setDelay = true, int delay = 0);

    int getPrimaryDelay();
    int getSecondaryDelay();
    std::string getPrimaryWeaponCategory() const;
    std::string getSecondaryWeaponCategory() const;

    std::string getWeaponVerb() const;
    std::string getWeaponVerbPlural() const;

    time_t getLTAttack() const;
    void modifyAttackDelay(int amt);
    void setAttackDelay(int newDelay);
    int getAttackDelay() const;
    int getBaseDamage() const;
    float getDamageReduction(const std::shared_ptr<Creature> & target) const; // How much is our damage reduced attacking the target
    AttackResult getAttackResult(const std::shared_ptr<Creature>& victim, const std::shared_ptr<Object>&  weapon = nullptr, int resultFlags = 0, int altSkillLevel = -1);

    bool kamiraLuck(const std::shared_ptr<Creature>&attacker);
    virtual int computeDamage(std::shared_ptr<Creature> victim, std::shared_ptr<Object>  weapon,
                              AttackType attackType, AttackResult& result, Damage& attackDamage,
                              bool computeBonus, int &drain, float multiplier = 1.0) = 0;
    bool canRiposte() const;
    bool canParry(const std::shared_ptr<Creature>& attacker);
    bool canDodge(const std::shared_ptr<Creature>& attacker);
    int dodge(const std::shared_ptr<Creature>& target);
    int parry(const std::shared_ptr<Creature>& target);
    double getFumbleChance(const std::shared_ptr<Object>&  weapon) const;
    double getCriticalChance(const int& difference) const;
    double getBlockChance(std::shared_ptr<Creature> attacker, const int& difference);
    double getGlancingBlowChance(const std::shared_ptr<Creature>& attacker, const int& difference) const;
    double getParryChance(const std::shared_ptr<Creature>& attacker, const int& difference);
    double getDodgeChance(const std::shared_ptr<Creature>& attacker, const int& difference);
    double getMisschanceModifier(const std::shared_ptr<Creature>& victim, double& missChance);
    double getMissChance(const int& difference);
    virtual int getWeaponSkill(std::shared_ptr<Object>  weapon = nullptr) const = 0;
    virtual int getDefenseSkill() const = 0;
    int adjustChance(const int &difference) const;
    static int computeBlock(int dmg);
    bool getsGroupExperience(const std::shared_ptr<Monster>&  target);
    bool canHit(const std::shared_ptr<Creature>& target, std::shared_ptr<Object>  weapon = nullptr, bool glow = true, bool showFail = true);
    bool doReflectionDamage(Damage pDamage, const std::shared_ptr<Creature>& target, ReflectedDamageType printZero=REFLECTED_NONE);
    static void simultaneousDeath(const std::shared_ptr<Creature>& attacker, std::shared_ptr<Creature> target, bool freeAttacker, bool freeTarget);
    bool canBeDrained() const;


    int spellFail(CastType how);
    bool isMageLich();
    void doFreeAction();
    bool noPotion(SpellData* spellData) const;
    int doMpCheck(int splno);
    int getTurnChance(const std::shared_ptr<Creature>& target);

// Special Attacks
    std::string getSpecialsFullList() const;
    bool useSpecial(SpecialAttack &attack, const std::shared_ptr<Creature> &victim);
    bool runSpecialAttacks(const std::shared_ptr<Creature>& victim); // Pick a special attack and do it on the target
    std::string getSpecialsList() const;
    SpecialAttack* addSpecial(std::string_view specialName);
    bool delSpecials();

    // Do Special should only be run from useSpecial, should not be called from elsewhere
    bool doSpecial(SpecialAttack &attack, const std::shared_ptr<Creature>& victim); // Do the selected attack on the given victim

// Traits
    bool doesntBreathe() const;
    bool immuneCriticals() const; // *
    bool isUndead() const; // *
    bool isPureArcaneCaster() const;
    bool isPureDivineCaster() const;
    bool isHybridArcaneCaster() const;
    bool isHybridDivineCaster() const;
    bool hasMp();
    bool isAntiGradius() const;
    bool countForWeightTrap() const;
    bool isRace(int r) const; // *
    bool isBrittle() const; // *
    bool isUnconscious() const; // *
    bool isBraindead() const; // *

    bool isHidden() const; // *
    bool canSpeak() const;

    bool negAuraRepel() const;
    bool canBuildObjects() const;
    bool canBuildMonsters() const;

    bool isPublicWatcher() const;
    bool isPet() const; // *
    bool isWatcher() const; // *
    bool isStaff() const; // *
    bool isCt() const; // *
    bool isDm() const; // *
    bool isAdm() const; // *

    bool isPureCaster() const;
    bool isHybridCaster() const;
    bool hatesEnemy(std::shared_ptr<Creature> enemy) const;

// Equipment / Inventory
    void addObj(const std::shared_ptr<Object>&  object);
    void delObj(std::shared_ptr<Object>  object, bool breakUnique=false, bool removeUnique=false, bool darkmetal=true, bool darkness=true, bool keep=false);
    void finishDelObj(const std::shared_ptr<Object>&  object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep);
    int getWeight() const;
    int maxWeight();
    bool tooBulky(int n) const;
    int getTotalBulk() const;
    int getMaxBulk() const;
    unsigned long getInventoryValue() const;
    void killDarkmetal();
    bool equip(const std::shared_ptr<Object>&  object, bool showMessage=true);
    std::shared_ptr<Object>  unequip(int wearloc, UnequipAction action = UNEQUIP_ADD_TO_INVENTORY, bool darkness=true, bool showEffect=true);
    void printEquipList(const std::shared_ptr<Player>& viewer);
    void checkDarkness();
    int countBagInv();
    int countInv(bool permOnly = false);

// Afflictions
    void poison(const std::shared_ptr<Creature>& enemy, int damagePerPulse, int duration);
    bool immuneToPoison() const; // *
    bool isPoisoned() const;
    bool curePoison();
    void disease(const std::shared_ptr<Creature>& enemy, int damagePerPulse);
    bool poisonedByMonster() const;
    bool poisonedByPlayer() const;
    bool immuneToDisease() const; // *
    bool isDiseased() const; // *
    bool cureDisease();
    bool removeCurse();
    bool isBlind() const; // *
    bool isNewVampire() const; // *
    bool isNewWerewolf() const; // *
    void makeVampire();
    bool willBecomeVampire() const;
    bool vampireCharmed(const std::shared_ptr<Player>& master);
    void clearMinions();
    bool addPorphyria(const std::shared_ptr<Creature>&killer, int chance);
    bool sunlightDamage();
    void makeWerewolf();
    bool willBecomeWerewolf() const;
    bool addLycanthropy(const std::shared_ptr<Creature>&killer, int chance);


// Get
    CreatureClass getClass() const; // *
    int getClassInt() const;
    int getSecondClassInt() const;
    unsigned short getLevel() const; // *
    short getAlignment() const; // *
    int getDeityAlignment() const;
    int getArmor() const; // *
    unsigned long getExperience() const; // *

    unsigned short getClan() const; // *
    mType getType() const;
    unsigned short getRace() const; // *
    unsigned short getDeity() const; // *
    Size getSize() const; // *
    int getAttackPower() const; // *
    std::string getDescription() const; // *
    std::string getVersion() const; // *
    unsigned short getPoisonDuration() const;
    unsigned short getPoisonDamage() const;
    Sex getSex() const;
    unsigned long getRealm(Realm r) const;
    std::string getPoisonedBy() const;
    unsigned short getDisplayRace() const;
    bool inJail() const;
    int getWillpower();

// Set
    void setClass(CreatureClass c); // *
    void setClan(unsigned short c); // *
    void setRace(unsigned short r); // *
    void addExperience(unsigned long e); // *
    void subExperience(unsigned long e); // *
    void setExperience(unsigned long e);
    void setLevel(unsigned short l, bool isDm=false);
    void setAlignment(short a);
    void subAlignment(unsigned short a); // *
    void shiftAlignment(short shift, bool silent=false);
    void setOppositeAlignment(bool silent=false);
    void setArmor(int a);
    void setAttackPower(int a);
    void setDeity(unsigned short d);
    void setSize(Size s);
    void setType(unsigned short t);
    void setType(mType t);
    void setDescription(std::string_view desc);
    void setVersion(std::string_view v = "");
    void setVersion(xmlNodePtr rootNode);
    void setPoisonDuration(unsigned short d);
    void setPoisonDamage(unsigned short d);
    void setSex(Sex sex);
    void setDeathType(DeathType d);
    void setPoisonedBy(std::string_view p);

    void setRealm(unsigned long num, Realm r);
    void addRealm(unsigned long num, Realm r);
    void subRealm(unsigned long num, Realm r);





// Miscellaneous
    std::string getHpDurabilityStr();
    std::string getHpProgressBar();

    bool pFlagIsSet(int flag) const;
    void pSetFlag(int flag);
    void pClearFlag(int flag);

    bool mFlagIsSet(int flag) const;
    void mSetFlag(int flag);
    void mClearFlag(int flag);


    bool flagIsSet(int flag) const;  // *
    void setFlag(int flag); // *
    void clearFlag(int flag); // *
    bool toggleFlag(int flag); // *

    bool spellIsKnown(int spell) const; // *
    void learnSpell(int spell); // *
    void forgetSpell(int spell); // *

    bool languageIsKnown(int lang) const; // *
    void learnLanguage(int lang); // *
    void forgetLanguage(int lang); // *

    bool inSameRoom(const std::shared_ptr<const Creature> &b) const;
    virtual int getAdjustedAlignment() const=0;
    bool checkDimensionalAnchor() const;
    bool checkStaff(const char *failStr, ...) const;
    int smashInvis(); // *
    bool unhide(bool show = true); // *
    void unmist(); // *
    void adjustStats();
    void fixLts();
    void doDispelMagic(int num=-1);
    bool changeSize(int oldStrength, int newStrength, bool enlarge);
    bool checkResistEnchantments(const std::shared_ptr<Creature>& caster, const std::string spell, bool print=false);
    bool isMagicallyHeld(bool print=false) const;
    void doCheckBreakMagicalHolds(std::shared_ptr<Creature>& attacker);

    bool addStatModifier(std::string_view statName, std::string_view modifierName, int modAmt, ModifierType modType);
    bool addStatModifier(std::string_view statName, StatModifier* statModifier);
    bool setStatDirty(std::string_view statName);
    Stat* getStat(std::string_view statName);

    // these handle total invisibility, no concealment (ie, being hidden)
    bool canSee(const std::shared_ptr<const MudObject> target, bool skip=false) const;

    bool canSeeRoom(const std::shared_ptr<BaseRoom>& room, bool p=false) const;
    bool canEnter(const std::shared_ptr<Exit>& ext, bool p=false, bool blinking=false) const;
    bool canEnter(const std::shared_ptr<UniqueRoom>& room, bool p=false) const;
    bool willFit(const std::shared_ptr<Object>&  object) const;
    bool canWield(const std::shared_ptr<Object>&  object, int n) const;
    bool canFlee(bool displayFail = false, bool checkTimer = true);
    bool canFleeToExit(const std::shared_ptr<Exit>& exit, bool skipScary=false, bool blinking=false);
    std::shared_ptr<Exit> getFleeableExit();
    std::shared_ptr<BaseRoom> getFleeableRoom(const std::shared_ptr<Exit>& exit);
    int flee(bool magicTerror=false, bool wimpyFlee = false);
    bool doFlee(bool magicTerror=false);

    bool isSitting() const;

    bool ableToDoCommand( cmd* cmnd=nullptr) const;
    void wake(const std::string& str = "", bool noise=false);
    void modifyDamage(const std::shared_ptr<Creature>& enemy, int dmgType, Damage& attackDamage, Realm pRealm=NO_REALM, const std::shared_ptr<Object>&  weapon=nullptr, short saveBonus=0, short offguard=OFFGUARD_REMOVE, bool computingBonus=false);
    bool checkResistPet(const std::shared_ptr<Creature>&pet, bool& resistPet, bool& immunePet, bool& vulnPet);

    void doHaggling(const std::shared_ptr<Creature>&vendor, const std::shared_ptr<Object>&  object, int trans);

    std::shared_ptr<BaseRoom> recallWhere();
    std::shared_ptr<BaseRoom> teleportWhere();
    Location getLimboRoom() const;
    Location getRecallRoom() const;
    int deleteFromRoom(bool delPortal=true);
    void applyMagicalArmor(Damage& dmg, int dmgType);

protected:
    virtual int doDeleteFromRoom(std::shared_ptr<BaseRoom> room, bool delPortal) = 0;
public:

    int doResistMagic(int dmg, const std::shared_ptr<Creature>& enemy=nullptr);
    virtual void pulseTick(long t) = 0;

    std::shared_ptr<MudObject> findTarget(int findWhere, int findFlags, const std::string& str, int val);
    std::shared_ptr<MudObject> findObjTarget(ObjectSet &set, int findFlags, const std::string& str, int val, int* match);
    //std::shared_ptr<MudObject> findTarget(cmd* cmnd, TargetType targetType, bool offensive);

    // New songs
    bool isPlaying() const;
    const Song* getPlaying();
    bool setPlaying(const Song* newSong, bool echo = true);
    bool stopPlaying(bool echo = true);
    bool pulseSong(long t);
    const Song* playing{};

    void donePaging() const;
    bool addStatModEffect(EffectInfo *effect);
    bool remStatModEffect(EffectInfo *effect);
    int numEnemyMonInRoom();
};


//--------------------------------------------------------------------------------
// #Player
//--------------------------------------------------------------------------------

//*********************************************************************
//                      Player
//*********************************************************************


/*
 * creatures.h
 *	 Header file for creature / monster / player classes
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 * 	Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 * 	   Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#ifndef CREATURES_H_
#define CREATURES_H_

#include "damage.h"
#include "threat.h"
#include "group.h"
#include "creatureStreams.h"

enum mType {
    INVALID        = -1,
    PLAYER          = 0,
    MONSTER         = 1,

    NPC             = 2,
    HUMANOID        = 2,

    GOBLINOID       = 3,
    MONSTROUSHUM    = 4,
    GIANTKIN        = 5,
    ANIMAL          = 6,
    DIREANIMAL      = 7,
    INSECT          = 8,
    INSECTOID       = 9,
    ARACHNID        = 10,
    REPTILE         = 11,
    DINOSAUR        = 12,
    AUTOMATON       = 13,
    AVIAN           = 14,
    FISH            = 15,
    PLANT           = 16,
    DEMON           = 17,
    DEVIL           = 18,
    DRAGON          = 19,
    BEAST           = 20,
    MAGICALBEAST    = 21,
    GOLEM           = 22,
    ETHEREAL        = 23,
    ASTRAL          = 24,
    GASEOUS         = 25,
    ENERGY          = 26,
    FAERIE          = 27,
    DEVA            = 28,
    ELEMENTAL       = 29,
    PUDDING         = 30,
    SLIME           = 31,
    UNDEAD          = 32,

    MAX_MOB_TYPES
};

#include "monType.h"

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

// prototype because calendar.h hasnt been called yet
class cDay;
class StartLoc;
class Monster;
class Recipe;
class TalkResponse;
class QuestInfo;
class QuestCompletion;
class QuestCatRef;
class SpellData;

// NOTE: It is important to use these defines for loading in an array of aray of *chars*
// ie: attack[3][30].  The size in the structure and the size passed to the loadStringArray
// function must be the same, this ensures it.  If creating any more arrays of this type use
// a similar define and use it in files-xml-read.c as well.
// -- Bane
#define CRT_ATTACK_LENGTH		30
#define CRT_KEY_LENGTH			20
#define CRT_MOVETYPE_LENGTH		52
#define CRT_FLAG_ARRAY_SIZE		32

#define NUM_ASSIST_MOB			8
#define NUM_ENEMY_MOB			4

#define DEL_ROOM_DESTROYED		1
#define DEL_PORTAL_DESTROYED	2
#define NUM_RESCUE			    2

class SkillGain;
class SpecialAttack;

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

#include "timer.h"


class CustomCrt {
public:
	CustomCrt();

	short	community;

	short	parents;
	short	brothers;
	short	sisters;

	short	social;
	short	education;
	short	height;
	short	weight;

	bstring hair;
	bstring eyes;
	bstring skin;


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

typedef std::list<Monster*> PetList;
typedef std::map<bstring, Skill*> SkillMap;
//*********************************************************************
//						Creature
//*********************************************************************

class Creature: public virtual MudObject, public Streamable, public Container, public Containable {

protected:
	void CopyCommon(const Creature& cr);
	void crtDestroy();
	void crtReset();

protected:
// Data
	unsigned short level;
	unsigned short cClass;
	unsigned short race;
	short alignment;
	mType type; // creature type
	unsigned int attackPower; // Attack power increases base damage of a creature
	unsigned int armor;
	unsigned long experience;
	unsigned long temp_experience; // Temp experience.
	unsigned short deity;
	Size size;
	unsigned short clan;
	unsigned short poison_dur;
	unsigned short poison_dmg;
	bstring description;
	bstring version; // Version of the mud this creature was saved under
	char flags[CRT_FLAG_ARRAY_SIZE]; // Max flags - 256
	unsigned long realm[MAX_REALM-1]; // Magic Spell realms
	char spells[32]; // more spells
	char quests[32]; // more quests
	static const short OFFGUARD_REMOVE;
	static const short OFFGUARD_NOREMOVE;
	static const short OFFGUARD_NOPRINT;
	DeathType deathtype;
	bstring poisonedBy;		// displayed to player, if this is a player, they get credit for pkill

	Group* group;
	GroupStatus groupStatus;


public:
// Constructors, Deconstructors, etc
	Creature();
	Creature(Creature& cr);
	Creature(const Creature& cr);
	Creature& operator=(const Creature& cr);
	//virtual bool operator< (const MudObject& t) const = 0;
	virtual ~Creature() {};

//	Monster* getMonster();
//	Player* getPlayer();
//

	virtual void upgradeStats() {};
	virtual Socket* getSock() const;
	Location getLocation();
	void delayedAction(bstring action, int delay, MudObject* target=0);
	void delayedScript(bstring script, int delay);

//	const Monster* getConstMonster() const;
//	const Player* getConstPlayer() const;

	Creature* getMaster();
	const Creature* getConstMaster() const;

	Player* getPlayerMaster();
	const Player* getConstPlayerMaster() const;


	bool isPlayer() const;
	bool isMonster() const;
	bool checkMp(int reqMp);
	bool checkResource(ResourceType resType, int resCost);
	void subResource(ResourceType resType, int resCost);
	void subMp(int reqMp);

	Creature* myTarget;
    PetList pets;
	std::list<Creature*> targetingThis;

	MagicType getCastingType() const;
	int doHeal(Creature* target, int amt, double threatFactor = 0.5);

	bstring doReplace(bstring fmt, const MudObject* actor=NULL, const MudObject* applier=NULL) const;

	void unApplyTongues();
	void unSilence();
	void unBlind();
	void stand();

	bool inSameRoom(Creature* target);


	Creature* findVictim(cmd* cmnd, int cmndNo, bool aggressive=true, bool selfOk=false, bstring noVictim="", bstring notFound="");
	Creature* findVictim(bstring toFind, int num, bool aggressive=true, bool selfOk=false, bstring noVictim="", bstring notFound="");
	Creature* findMagicVictim(bstring toFind, int num, SpellData* spellData, bool aggressive=true, bool selfOk=false, bstring noVictim="", bstring notFound="");

	bool hasAttackableTarget();
	Creature* getTarget();
	Creature* addTarget(Creature* toTarget);
	void checkTarget(Creature* toTarget);
	void addTargetingThis(Creature* targeter);
	void clearTarget(bool clearTargetsList = true);
	void clearTargetingThis(Creature* targeter);

	long getLTLeft(int myLT, long t = -1); // gets the time left on a LT
	void setLastTime(int myLT, long t, long interval); // Sets a LT



public:
// Data
	bstring plural;
	std::map<bstring, long> factions;
	SkillMap skills;
	char key[3][CRT_KEY_LENGTH];
	short fd; // Socket number
	short current_language;
	long proficiency[6]; // Weapon proficiencies: 6 now..added cleaving weapons
	int afterProf;
	Dice damage;
	Money coins;
	//CatRef room;
#define 				NUMHITS quests[0]
	short questnum; // Quest fulfillment number (M)
	Object *ready[MAXWEAR];// Worn/readied items
	otag *first_obj; // List of inventory
	//etag *first_enm; // List of enemies
	ttag *first_tlk; // List of talk responses

	struct saves saves[6]; // Saving throws struct. POI, DEA, BRE, MEN, SPL, x, x
	char languages[16];
	char movetype[3][CRT_MOVETYPE_LENGTH]; // Movement types..."flew..oozed...etc.."
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
	struct lasttime lasttime[128]; // added more timers

	// future new tic amount code
	struct tic hpTic;
	struct tic mpTic;
	struct tic ppTic;
	struct tic rpTic;

	struct spellTimer spellTimer[16]; // spell effect timers (specific to magic)
	char misc[21]; // miscellaneous space

	//EffectList effects; // List of all effects on this creature
	Timer attackTimer;
	std::list<SpecialAttack*> specials; // List of all special attack this creature has
	std::list<bstring> minions; // vampire minions


	Location currentLocation;
	Location previousRoom; // last room they were in
	void setPreviousRoom();

// Groups & Pets

    void setGroup(Group* newGroup);
    void setGroupStatus(GroupStatus newStatus);

    Group* getGroup(bool inGroup = true);
    GroupStatus getGroupStatus();
    Creature* getGroupLeader();

    void addToGroup(Group* toJoin, bool announce = true);
    void createGroup(Creature* crt);
    bool removeFromGroup(bool announce = true);

    bool inSameGroup(Creature* target);

    void dismissPet(Monster* toDismiss);
    void dismissAll();
    void displayPets();
	void addPet(Monster* newPet, bool setPetFlag = true);
	void delPet(Monster* toDel);
	bool hasPet() const;

	Monster* findPet(Monster* toFind);
	Monster* findPet(bstring pName, int pNum);

// XML: loading and saving
	int saveToXml(xmlNodePtr rootNode, int permOnly, LoadType saveType, bool saveID = true) const;
    void savePets(xmlNodePtr curNode) const;
	void saveSkills(xmlNodePtr rootNode) const;
	void saveFactions(xmlNodePtr rootNode) const;
	void saveAttacks(xmlNodePtr rootNode) const;
	int readFromXml(xmlNodePtr rootNode);
	void loadAttacks(xmlNodePtr rootNode);
	void loadFactions(xmlNodePtr rootNode);
	bool loadFaction(xmlNodePtr rootNode);
	void loadSkills(xmlNodePtr rootNode);
	void loadStats(xmlNodePtr curNode);

	bool pulseEffects(time_t t);

	void convertOldEffects();
	bool convertFlag(int flag);
	bool convertToEffect(const bstring& effect, int flag, int lt);
	bool addStatModEffect(EffectInfo* effect);
	bool remStatModEffect(EffectInfo* effect);
	void removeStatEffects();
	bool doPetrificationDmg();
	bool willIgnoreIllusion() const;
	bool isInvisible() const; // *

// Skills
	bool knowsSkill(const bstring& skillName) const; // *
	double getSkillLevel(const bstring& skillName) const; // *
	double getSkillGained(const bstring& skillName) const; // *
	double getTradeSkillGained(const bstring& skillName) const; // *
	Skill* getSkill(const bstring& skillName) const;
	void addSkill(const bstring& skillName, int gained); // *
	void remSkill(const bstring& skillName); // *
	void checkSkillsGain(std::list<SkillGain*>::const_iterator begin, std::list<SkillGain*>::const_iterator end, bool setToLevel = false);
	void checkImprove(const bstring& skillName, bool success, int attribute = INT, int bns = 0); // *
	bool setSkill(const bstring skill, int gained); // *

// Formatting
	virtual void escapeText() {};
	bstring getCrtStr(const Creature* viewer = NULL, int flags = 0, int num = 0) const;
	bstring statCrt(int statFlags);
	int displayFlags() const;
	bstring alignColor() const;
	bstring fullName() const;
	const char *hisHer() const;
	const char *himHer() const;
	const char *heShe() const;
	const char *upHisHer() const;
	const char *upHeShe() const;
	void pleaseWait(long duration) const;
	void pleaseWait(int duration) const;
	void pleaseWait(double duration) const;
	const char* getStatusStr(int dmg=0);
	virtual const bstring customColorize(bstring text, bool caret=true) const = 0;

	void bPrint(bstring toPrint) const;
	void print(const char *fmt, ...) const;
	void printColor(const char *fmt, ...) const;
	virtual void vprint(const char *fmt, va_list ap) const {};

// Combat & Death
	bool checkDie(Creature *killer); // *
	bool checkDie(Creature *killer, bool &freeTarget); // *
	int checkDieRobJail(Monster *killer); // *
	int checkDieRobJail(Monster *killer, bool &freeTarget); // *
	void checkDoctorKill(Creature *victim);
	void die(Creature *killer); // *
	void die(Creature *killer, bool &freeTarget); // *
	void clearAsPetEnemy();
	virtual void gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp = false) {} ;
	void adjustExperience(Monster* victim, int& expAmount, int& holidayExp);
	int doWeaponResist(int dmg, bstring weaponCategory) const;
	int doDamage(Creature* target, int dmg, DeathCheck shouldCheckDie = CHECK_DIE, DamageType dmgType = PHYSICAL_DMG);
	int doDamage(Creature* target, int dmg, DeathCheck shouldCheckDie, DamageType dmgType, bool &freeTarget);
	int chkSave(short savetype, Creature* target, short bns);
	int castWeapon(Creature* target, Object* weapon, bool &meKilled);
	void castDelay(long delay);
	void attackDelay(long delay);
	void stun(int delay);
	int doLagProtect();
	bool hasCharm(bstring charmed);
	bool inCombat(bool countPets) const;
	bool inCombat(const Creature* target=0, bool countPets=0) const;
	bool canAttack(Creature* target, bool stealing=false);
	int checkRealmResist(int dmg, Realm realm) const;
	void knockUnconscious(long duration);
	void clearAsEnemy();
	bool checkAttackTimer(bool displayFail = true);
	void updateAttackTimer(bool setDelay = true, int delay = 0);
	time_t getLTAttack() const;
	void modifyAttackDelay(int amt);
	void setAttackDelay(int newDelay);
	int getAttackDelay() const;
	unsigned int getBaseDamage() const;
	float getDamageReduction(const Creature* target) const; // How much is our damage reduced attacking the target
	AttackResult getAttackResult(Creature* victim, const Object* weapon = NULL, int resultFlags = 0, int altSkillLevel = -1);
	bool kamiraLuck(Creature *attacker);
	virtual int computeDamage(Creature* victim, Object* weapon,
			AttackType attackType, AttackResult& result, Damage& attackDamage,
			bool computeBonus, int& drain, float multiplier = 1.0) = 0;
	bool canRiposte() const;
	bool canParry(Creature* attacker);
	bool canDodge(Creature* attacker);
	int dodge(Creature* target);
	int parry(Creature* target);
	double getFumbleChance(const Object* weapon);
	double getCriticalChance(const int& difference);
	double getBlockChance(Creature* attacker, const int& difference);
	double getGlancingBlowChance(Creature* attacker, const int& difference);
	double getParryChance(Creature* attacker, const int& difference);
	double getDodgeChance(Creature* attacker, const int& difference);
	double getMissChance(const int& difference);
	virtual int getWeaponSkill(const Object* weapon = NULL) const = 0;
	virtual int getDefenseSkill() const = 0;
	int adjustChance(const int &difference);
	int computeBlock(int dmg);
	bool getsGroupExperience(Monster* target);
	bool canHit(Creature* target, Object* weapon = NULL, bool glow = true, bool showFail = true);
	bool doReflectionDamage(Damage damage, Creature* target, ReflectedDamageType printZero=REFLECTED_NONE);
	static void simultaneousDeath(Creature* attacker, Creature* target, bool freeAttacker, bool freeTarget);
	bool canBeDrained() const;

// Special Attacks
	bstring getSpecialsFullList() const;
	bool useSpecial(const bstring& special, Creature* victim);
	bool useSpecial(SpecialAttack* attack, Creature* victim);
	bool runOpeners(Creature* victim); // Run any opening attacks
	bool runSpecialAttacks(Creature* victim); // Pick a special attack and do it on the target
	bstring getSpecialsList() const;
	SpecialAttack* addSpecial(const bstring specialName);
	bool delSpecials();
	SpecialAttack* getSpecial(const bstring& special);

	// Do Special should only be run from useSpecial, should not be called from elsewhere
	bool doSpecial(SpecialAttack* attack, Creature* victim); // Do the selected attack on the given victim

// Traits
	bool doesntBreathe() const;
	bool immuneCriticals() const; // *
	bool isUndead() const; // *
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

// Equipment / Inventory
	void addObj(Object* object, bool resetUniqueId=true);
	void delObj(Object* object, bool breakUnique=false, bool removeUnique=false, bool darkmetal=true, bool darkness=true, bool keep=false);
	void finishDelObj(Object* object, bool breakUnique, bool removeUnique, bool darkmetal, bool darkness, bool keep);
	int getWeight() const;
	int maxWeight();
	bool tooBulky(int n) const;
	int getTotalBulk() const;
	int getMaxBulk() const;
	unsigned long getInventoryValue() const;
	void killDarkmetal();
	bool equip(Object* object, bool showMessage=true, bool resetUniqueId=true);
	Object* unequip(int wearloc, UnequipAction action = UNEQUIP_ADD_TO_INVENTORY, bool darkness=true, bool showEffect=true);
	void printEquipList(const Player* viewer);
	void checkDarkness();

// Afflictions
	void poison(Creature* enemy, unsigned int damagePerPulse, unsigned int duration);
	bool immuneToPoison() const; // *
	bool isPoisoned() const;
	bool curePoison();
	void disease(Creature* enemy, unsigned int damagePerPulse);
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
	bool vampireCharmed(Player* master);
	void clearMinions();
	bool addPorphyria(Creature *killer, int chance);
	bool sunlightDamage();
	void makeWerewolf();
	bool willBecomeWerewolf() const;
	bool addLycanthropy(Creature *killer, int chance);

// Get
	const char* getName() const; // *
	unsigned short getClass() const; // *
	unsigned short getLevel() const; // *
	short getAlignment() const; // *
	unsigned int getArmor() const; // *
	unsigned long getExperience() const; // *

	unsigned short getClan() const; // *
	mType getType() const;
	unsigned short getRace() const; // *
	unsigned short getDeity() const; // *
	Size getSize() const; // *
	unsigned int getAttackPower() const; // *
	bstring getDescription() const; // *
	bstring getVersion() const; // *
	unsigned short getPoisonDuration() const;
	unsigned short getPoisonDamage() const;
	Sex getSex() const;
	unsigned long getRealm(Realm r) const;
	bstring getPoisonedBy() const;
	unsigned short getDisplayRace() const;
	bool inJail() const;

// Set
	void setClass(unsigned short c); // *
	void setClan(unsigned short c); // *
	void setRace(unsigned short r); // *
	void addExperience(unsigned long e); // *
	void subExperience(unsigned long e); // *
	void setExperience(unsigned long e);
	void setLevel(unsigned short l, bool isDm=false);
	void setAlignment(short a);
	void subAlignment(unsigned short a); // *
	void setArmor(unsigned int a);
	void setAttackPower(unsigned int a);
	void setDeity(unsigned short d);
	void setSize(Size s);
	void setType(unsigned short t);
	void setType(mType t);
	void setDescription(const bstring& desc);
	void setVersion(bstring v = "");
	void setVersion(xmlNodePtr rootNode);
	void setPoisonDuration(unsigned short d);
	void setPoisonDamage(unsigned short d);
	void setSex(Sex sex);
	void setDeathType(DeathType d);
	void setPoisonedBy(const bstring& p);

	void setRealm(unsigned long num, Realm r);
	void addRealm(unsigned long num, Realm r);
	void subRealm(unsigned long num, Realm r);





// Miscellaneous
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

	bool inSameRoom(const Creature *b) const;
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

	bool addStatModifier(bstring statName, bstring modifierName, int modAmt, ModifierType modType);
	bool addStatModifier(bstring statName, StatModifier* statModifier);
	bool setStatDirty(bstring statName);
	Stat* getStat(bstring statName);

	// these handle total invisibility, no concealment (ie, being hidden)
	bool canSee(const BaseRoom* room, bool p=false) const;
	bool canSee(const Object* object) const;
	bool canSee(const Exit *ext) const;
	bool canSee(const Creature* target, bool skip=false) const;
	bool canEnter(const Exit *ext, bool p=false, bool blinking=false) const;
	bool canEnter(const UniqueRoom* room, bool p=false) const;
	bool willFit(const Object* object) const;
	bool canWield(const Object* object, int n) const;
	bool canFlee();
	bool canFleeToExit(const Exit *exit, bool skipScary=false, bool blinking=false);
	Exit* getFleeableExit();
	BaseRoom* getFleeableRoom(Exit* exit);
	int flee(bool magicTerror=false);

	bool ableToDoCommand(const cmd* cmnd=NULL) const;
	void wake(bstring str = "", bool noise=false);
	void modifyDamage(Creature* enemy, int atype, Damage& damage, Realm realm=NO_REALM, Object* weapon=0, int saveBonus=0, short offguard=OFFGUARD_REMOVE, bool computingBonus=false);
	bool checkResistPet(Creature *pet, bool& resistPet, bool& immunePet, bool& vulnPet);

	void doHaggling(Creature *vendor, Object* object, int trans);

	BaseRoom* recallWhere();
	BaseRoom* teleportWhere();
	Location getLimboRoom() const;
	Location getRecallRoom() const;
	int deleteFromRoom(bool delPortal=true);
protected:
	virtual int doDeleteFromRoom(BaseRoom* room, bool delPortal) = 0;
public:

	int doResistMagic(int dmg, Creature* enemy=0);
	virtual void pulseTick(long t) = 0;

	//MudObject* findTarget(cmd* cmnd, TargetType targetType);

	// New songs
	bool isPlaying();
	Song* getPlaying();
	bool setPlaying(Song* newSong, bool echo = true);
	bool stopPlaying(bool echo = true);
	bool pulseSong(long t);
	Song* playing;

};

//*********************************************************************
//						Monster
//*********************************************************************

class Monster : public Creature {
protected:
	void doCopy(const Monster& cr);
	void reset();
	int doDeleteFromRoom(BaseRoom* room, bool delPortal);

public:
// Constructors, Deconstructors, etc
	Monster();
	Monster(Monster& cr);
	Monster(const Monster& cr);
	Monster& operator=(const Monster& cr);
	bool operator< (const Monster& t) const;
	~Monster();
	void readXml(xmlNodePtr curNode);
	void saveXml(xmlNodePtr curNode) const;
	int saveToFile();
	void validateId();
	void upgradeStats();

protected:
// Data
	unsigned short updateAggro;
	unsigned short loadAggro;
	unsigned short numwander;
	unsigned short magicResistance;
	unsigned short mobTrade;
	int skillLevel;
	unsigned short maxLevel;
	unsigned short cast;
	Realm baseRealm; // For pets/elementals -> What realm they are
	bstring primeFaction;
	bstring talk;
	// Not giving monsters skills right now, so store it on their character
	int weaponSkill;
	int defenseSkill;

	Creature* myMaster;

public:
// Data
	char last_mod[25]; // last staff member to modify creature.
	char ttalk[72];
	char aggroString[80];
	char attack[3][CRT_ATTACK_LENGTH];
	std::list<TalkResponse*> responses;
	char cClassAggro[4]; // 32 max
	char raceAggro[4]; // 32 max
	char deityAggro[4]; // 32 max
	CatRef info;
	CatRef assist_mob[NUM_ASSIST_MOB];
	CatRef enemy_mob[NUM_ENEMY_MOB];
	CatRef jail;
	CatRef rescue[NUM_RESCUE];
	Carry carry[10]; // Random items monster carries


// Combat & Death
//	int addEnmName(const char* enemy);
//	int addEnmCrt(Creature* target, bool print=false);
//	int delEnmCrt(const char* enemy, const char* owner = 0);
//	void endEnmCrt(const char* enemy);
//	void addEnmDmg(const Creature* enemy, int dmg);
//	bool isEnmCrt(const char* enemy) const;
    bool addEnemy(Creature* target, bool print=false);
    long clearEnemy(Creature* target);

    bool isEnemy(const Creature* target) const;
    bool hasEnemy() const;

    long adjustThreat(Creature* target, long modAmt, double threatFactor = 1.0);
    long adjustContribution(Creature* target, long modAmt);
    void clearEnemyList();


    Creature* getTarget(bool sameRoom=true);
	bool nearEnemy(const Creature* target) const;

	ThreatTable* threatTable;

    void setMaster(Creature* pMaster);
    Creature* getMaster() const;

	Player* whoToAggro() const;
	bool willAggro(const Player *player) const;
	int getAdjustedAlignment() const;
	int toJail(Player* player);
	void dieToPet(Monster *killer, bool &freeTarget);
	void dieToMonster(Monster *killer, bool &freeTarget);
	void dieToPlayer(Player *killer, bool &freeTarget);
	void mobDeath(Creature *killer=0);
	void mobDeath(Creature *killer, bool &freeTarget);
	void finishMobDeath(Creature *killer);
	void logDeath(Creature *killer);
	void dropCorpse(Creature *killer);
	void cleanFollow(Creature *killer);
	void distributeExperience(Creature *killer);
	void monsterCombat(Monster *target);
	bool nearEnemy() const;
	bool nearEnmPly() const;
	bool checkEnemyMobs();
	int updateCombat();
	bool checkAssist();
	bool willAssist(const Monster *victim) const;
	void adjust(int buffswitch);
	bool tryToDisease(Creature* target, SpecialAttack* attack = NULL);
	bool tryToStone(Creature* target, SpecialAttack* attack = NULL);
	bool tryToPoison(Creature* target, SpecialAttack* attack = NULL);
	bool tryToBlind(Creature* target, SpecialAttack* attack = NULL);
	bool tryToConfuse(Creature* target, SpecialAttack* attack = NULL);
	int zapMp(Creature *victim, SpecialAttack* attack = NULL);
	int petrify(Player *victim);
	void regenerate();
	int steal(Player *victim);
	void berserk();
	int summonMobs(Creature *victim);
	int castSpell(Creature *target);
	bool petCaster();
	int mobDeathScream();
	bool possibleEnemy();
	int doHarmfulAuras();
	void validateAc();
	bool isRaceAggro(int x, bool checkInvert) const;
	void setRaceAggro(int x);
	void clearRaceAggro(int x);
	bool isClassAggro(int x, bool checkInvert) const;
	void setClassAggro(int x);
	void clearClassAggro(int x);
	bool isDeityAggro(int x, bool checkInvert) const;
	void setDeityAggro(int x);
	void clearDeityAggro(int x);
	void gainExperience(Monster* victim, Creature* killer, int expAmount,
		bool groupExp = false);
	int powerEnergyDrain(Creature *victim, SpecialAttack* attack = NULL);
	int computeDamage(Creature* victim, Object* weapon, AttackType attackType,
		AttackResult& result, Damage& attackDamage, bool computeBonus,
		int& drain, float multiplier = 1.0);
	int mobWield();
	int checkScrollDrop();
	int grabCoins(Player* player);
	bool isEnemyMob(const Monster* target) const;
	void diePermCrt();

// Get
	unsigned short getMobTrade() const;
	int getSkillLevel() const;
	unsigned int getMaxLevel() const;
	unsigned short getNumWander() const;
	unsigned short getLoadAggro() const;
	unsigned short getUpdateAggro() const;
	unsigned short getCastChance() const;
	unsigned short getMagicResistance() const;
	bstring getPrimeFaction() const;
	bstring getTalk() const;
	int getWeaponSkill(const Object* weapon = NULL) const;
	int getDefenseSkill() const;

// Set
	void setMaxLevel(unsigned short l);
	void setCastChance(unsigned short c);
	void setMagicResistance(unsigned short m);
	void setLoadAggro(unsigned short a);
	void setUpdateAggro(unsigned short a);
	void setNumWander(unsigned short n);
	void setSkillLevel(int l);
	void setMobTrade(unsigned short t);
	void setPrimeFaction(bstring f);
	void setTalk(bstring t);
	void setWeaponSkill(int amt);
	void setDefenseSkill(int amt);


// Miscellaneous
	bool swap(Swap s);
	bool swapIsInteresting(Swap s) const;
	
	void killUniques();
	void escapeText();
	void convertOldTalks();
	void addToRoom(BaseRoom* room, int num=1);

	void checkSpellWearoff();
	void checkScavange(long t);
	int checkWander(long t);
	bool canScavange(Object* object);

	bool doTalkAction(Player* target, bstring action);
	void sayTo(const Player* player, const bstring& message);
	void pulseTick(long t);
	void beneficialCaster();
	int initMonster(bool loadOriginal = false, bool prototype = false);

	int mobileCrt();
	void getMobSave();
	int getNumMobs() const;
	void clearMobInventory();
	int cleanMobForSaving();

	Realm getBaseRealm() const;
	void setBaseRealm(Realm toSet);
	const bstring customColorize(bstring text, bool caret=true) const;
};

//--------------------------------------------------------------------------------
// #Player
//--------------------------------------------------------------------------------

#include "statistics.h"

//*********************************************************************
//						Player
//*********************************************************************

class Player : public Creature {
protected:
	void doCopy(const Player& cr);
	void reset();

	bool inList(const std::list<bstring>* list, bstring name) const;
	bstring showList(const std::list<bstring>* list) const;
	void addList(std::list<bstring>* list, bstring name);
	void delList(std::list<bstring>* list, bstring name);
	int doDeleteFromRoom(BaseRoom* room, bool delPortal);
	void finishAddPlayer(BaseRoom* room);
	static bstring hashPassword(bstring pass);
	long getInterest(long principal, double annualRate, long seconds);

public:
// Constructors, Deconstructors, etc
	Player();
	Player(Player& cr);
	Player(const Player& cr);
	Player& operator=(const Player& cr);
	bool operator< (const Player& t) const;
	~Player();
	int save(bool updateTime=false, LoadType saveType=LS_NORMAL);
	int saveToFile(LoadType saveType=LS_NORMAL);
	void loadAnchors(xmlNodePtr curNode);
	void readXml(xmlNodePtr curNode);
	void saveXml(xmlNodePtr curNode) const;
	void bug(const char *fmt, ...) const;
	void validateId();
	void upgradeStats();

protected:
// Data
	char customColors[CUSTOM_COLOR_SIZE];
	unsigned short warnings;
	unsigned short actual_level;
	unsigned short tickDmg;
	unsigned short negativeLevels;
	unsigned short guild; // Guild this player is in
	unsigned short guildRank; // Rank this player holds in their guild
	unsigned short cClass2;
	unsigned short wimpy;
	short barkskin;
	unsigned short pkin;
	unsigned short pkwon;
	int wrap;
	int luck;
	int ansi;
	unsigned short weaponTrains; // Number of weapons they are able to train in
	long lastLogin;
	long lastInterest;
	unsigned long timeout;
	bstring lastPassword;
	bstring afflictedBy;	// used to determine master with vampirism/lycanthropy
	int tnum[5];
	std::list<bstring> ignoring;
	std::list<bstring> gagging;
	std::list<bstring> refusing;
	std::list<bstring> dueling;
	std::list<bstring> maybeDueling;
	std::list<bstring> watching;
	bstring password;
	bstring title;
	bstring tempTitle;	// this field is purposely not saved
	Socket* mySock;
	bstring surname;
	bstring oldCreated;
	long created;
	bstring lastCommand;
	bstring lastCommunicate;
	bstring forum;		// forum account this character is associated with
	char songs[32];
	struct vstat tstat;
	Anchor *anchor[MAX_DIMEN_ANCHORS];
	cDay *birthday;
	Monster* alias_crt;
	std::list<CatRef> roomExp; // rooms they have gotten exp from
	unsigned short thirst;
	Object* lastPawn;
	int uniqueObjId;
public:
// Data
	std::list<CatRef> storesRefunded;   // Shops the player has refunded an item in
	                                    // (No haggling allowed until a full priced purchase has been made)
	std::list<CatRef> objIncrease; // rooms they have gotten exp from

	// TODO: Rework first_charm and get rid of etag
	etag *first_charm;
	int *scared_of;

	std::list<CatRef> lore;
	std::list<int> recipes;
	std::map<int, QuestCompletion*> questsInProgress;
	std::map<int,int> questsCompleted;	  // List of all quests we've finished and how many times

	Money bank;
	Stat focus; // Battle focus points for fighters
	CustomCrt custom;
	Location bound; // the room the player is bound to
	Statistics statistics;
	Range	bRange[MAX_BUILDER_RANGE];


// Combat & Death
	int computeAttackPower();
	void dieToPet(Monster *killer);
	void dieToMonster(Monster *killer);
	void dieToPlayer(Player *killer);
	void loseExperience(Monster *killer);
	void dropEquipment(bool dropAll=false, Socket* killSock = NULL);
	void dropBodyPart(Player *killer);
	bool isGuildKill(const Player *killer) const;
	bool isClanKill(const Player *killer) const;
	bool isGodKill(const Player *killer) const;
	int guildKill(Player *killer);
	int godKill(Player *killer);
	int clanKill(Player *killer);
	int checkLevel();
	void updatePkill(Player *killer);
	void logDeath(Creature *killer);
	void resetPlayer(Creature *killer);
	void getPkilled(Player *killer, bool dueling, bool reset=true);
	void die(DeathType dt);
	bool dropWeapons();
	int checkPoison(Creature* target, Object* weapon);
	int attackCreature(Creature *victim, AttackType attackType = ATTACK_NORMAL);
	void adjustAlignment(Monster *victim);
	void checkWeaponSkillGain();
	void loseAcid();
	void dissolveItem(Creature* creature);
	int computeDamage(Creature* victim, Object* weapon, AttackType attackType,
		AttackResult& result, Damage& attackDamage, bool computeBonus,
		int& drain, float multiplier = 1.0);
	int packBonus();
	int getWeaponSkill(const Object* weapon = NULL) const;
	int getDefenseSkill() const;
	void damageArmor(int dmg);
	void checkArmor(int wear);
	void gainExperience(Monster* victim, Creature* killer, int expAmount, bool groupExp = false);
	void disarmSelf();
	int lagProtection();
	void computeAC();
	void alignAdjustAcThaco();
	void checkOutlawAggro();
	bstring getUnarmedWeaponSkill() const;
	double winterProtection() const;
	bool isHardcore() const;
	bool canMistNow() const;

// Lists
	bool isIgnoring(bstring name) const;
	bool isGagging(bstring name) const;
	bool isRefusing(bstring name) const;
	bool isDueling(bstring name) const;
	bool isWatching(bstring name) const;
	void clearIgnoring();
	void clearGagging();
	void clearRefusing();
	void clearDueling();
	void clearMaybeDueling();
	void clearWatching();
	void addIgnoring(bstring name);
	void addGagging(bstring name);
	void addRefusing(bstring name);
	void addDueling(bstring name);
	void addMaybeDueling(bstring name);
	void addWatching(bstring name);
	void delIgnoring(bstring name);
	void delGagging(bstring name);
	void delRefusing(bstring name);
	void delDueling(bstring name);
	void delMaybeDueling(bstring name);
	void delWatching(bstring name);
	bstring showIgnoring() const;
	bstring showGagging() const;
	bstring showRefusing() const;
	bstring showDueling() const;
	bstring showWatching() const;
    void addRefundedInStore(CatRef& store);
    void removeRefundedInStore(CatRef& store);
    bool hasRefundedInStore(CatRef& store);

// Quests
	bool addQuest(QuestInfo* toAdd);
	bool hasQuest(int questId) const;
	bool hasQuest(const QuestInfo *quest) const;
	bool hasDoneQuest(int questId) const;
	void updateMobKills(Monster* monster);
	int countItems(const QuestCatRef& obj);
	void updateItems(Object* object);
	void updateRooms(UniqueRoom* room);
	void saveQuests(xmlNodePtr rootNode) const;
	bool questIsSet(int quest) const;
	void setQuest(int quest);
	void clearQuest(int quest);

// Get
	unsigned short getSecondClass() const;
	unsigned short getGuild() const;
	unsigned short getGuildRank() const;
	unsigned short getActualLevel() const;
	unsigned short getNegativeLevels() const;
	unsigned short getWimpy() const;
	unsigned short getTickDamage() const;
	unsigned short getWarnings() const;
	unsigned short getPkin() const;
	unsigned short getPkwon() const;
	bstring getBankDisplay() const;
	bstring getCoinDisplay() const;
	int getWrap() const;

	short getLuck() const;
	unsigned short getWeaponTrains() const;
	long getLastLogin() const;
	long getLastInterest() const;
	bstring getLastPassword() const;
	bstring getAfflictedBy() const;
	bstring getTitle() const;
	bstring getCustomTitle() const;
	bstring getTempTitle() const;
	bstring getLastCommunicate() const;
	bstring getLastCommand() const;
	bstring getSurname() const;
	bstring getForum() const;
	long getCreated() const;
	bstring getCreatedStr() const;
	Monster* getAlias() const;
	cDay* getBirthday() const;
	bstring getAnchorAlias(int i) const;
	bstring getAnchorRoomName(int i) const;
	const Anchor* getAnchor(int i) const;
	bool hasAnchor(int i) const;
	bool isAnchor(int i, const BaseRoom* room) const;
	unsigned short getThirst() const;
	bstring getCustomColor(CustomColor i, bool caret) const;
	int numDiscoveredRooms() const;
	int getUniqueObjId() const;

// Set
	void setTickDamage(unsigned short t);
	void setWarnings(unsigned short w);
	void addWarnings(unsigned short w);
	void subWarnings(unsigned short w);
	void setWimpy(unsigned short w);
	void setActualLevel(unsigned short l);
	void setSecondClass(unsigned short c);
	void setGuild(unsigned short g);
	void setGuildRank(unsigned short g);
	void setNegativeLevels(unsigned short l);
	void setLuck(int l);
	void setWeaponTrains(unsigned short t);
	void subWeaponTrains(unsigned short t);
	void setLostExperience(unsigned long e);
	void setLastPassword(bstring p);
	void setAfflictedBy(bstring a);
	void setLastLogin(long l);
	void setLastInterest(long l);
	void setTitle(bstring newTitle);
	void setTempTitle(bstring newTitle);
	void setLastCommunicate(bstring c);
	void setLastCommand(bstring c);
	void setCreated();
	void setSurname(bstring s);
	void setForum(bstring f);
	void setAlias(Monster* m);
	void setBirthday();
	void delAnchor(int i);
	void setAnchor(int i, bstring a);
	void setThirst(unsigned short t);
	void setCustomColor(CustomColor i, char c);
	int setWrap(int newWrap);

// Factions
	int getFactionStanding(bstring factionStr) const;
	bstring getFactionMessage(bstring factionStr) const;
	void adjustFactionStanding(const std::map<bstring, long>& factionList);

// Staff
	bool checkRangeRestrict(CatRef cr, bool reading=true) const;
	bool checkBuilder(UniqueRoom* room, bool reading=true) const;
	bool checkBuilder(CatRef cr, bool reading=true) const;
	void listRanges(const Player* viewer) const;
	void initBuilder();
	bool builderCanEditRoom(bstring action);

// Movement
	Location getBound();
	void bind(const StartLoc* location);
	void dmPoof(BaseRoom* room, BaseRoom *newRoom);
	void doFollow(BaseRoom* oldRoom);
	void doPetFollow();
	void doRecall(int roomNum = -1);
	void addToSameRoom(Creature* target);
	void addToRoom(BaseRoom* room);
	void addToRoom(AreaRoom* aUoom);
	void addToRoom(UniqueRoom* uRoom);
	bool showExit(const Exit* exit, int magicShowHidden=0) const;
	int checkTraps(UniqueRoom* room, bool self=true, bool isEnter=true);
	int doCheckTraps(UniqueRoom* room);
	bool checkHeavyRestrict(const bstring& skill) const;

// Recipes
	void unprepareAllObjects() const;
	void removeItems(const std::list<CatRef>* list, int numIngredients);
	void learnRecipe(int id);
	void learnRecipe(Recipe* recipe);
	bool knowsRecipe(int id) const;
	bool knowsRecipe(Recipe* recipe) const;
	Recipe* findRecipe(cmd* cmnd, bstring skill, bool* searchRecipes, Size recipeSize=NO_SIZE, int numIngredients=1) const;

// Stats & Ticking
	long tickInterval(Stat& stat, bool fastTick, bool deathSickness, bool vampAndDay, const bstring& effectName);
	void pulseTick(long t);
	int getHpTickBonus() const;
	int getMpTickBonus() const;
	void calcStats(vstat sendStat, vstat *toStat);
	void changeStats();
	void changingStats(bstring str);
	void decreaseFocus();
	void increaseFocus(FocusAction action, int amt = 0, Creature* target = NULL);
	void clearFocus();
	bool doPlayerHarmRooms();
	bool doDoTs();
	void loseRage();
	void losePray();
	void loseFrenzy();
	bool statsAddUp() const;
	bool canDefecate() const;

// Formating
	bstring getWhoString(bool whois=false, bool color=true, bool ignoreIllusion=false) const;
	bstring getTimePlayed() const;
	bstring consider(Creature* creature) const;
	int displayCreature(Creature* target);
	void sendPrompt();
	void defineColors();
	void setSockColors();
	void vprint(const char *fmt, va_list ap) const;
	void escapeText();
	bstring getClassString() const;
	void score(const Player* viewer);
	void information(const Player* viewer=0, bool online=true);
	void showAge(const Player* viewer) const;
	const bstring customColorize(bstring text, bool caret=true) const;
	void resetCustomColors();

// Misellaneous
	bool swap(Swap s);
	bool swapIsInteresting(Swap s) const;
	
	void doRemove(int i, bool resetUniqueId=true);
	int getAge() const;
	unsigned long expToLevel() const;
	bstring expToLevel(bool addX) const;
	bstring expInLevel() const;
	bstring expForLevel() const;
	bstring expNeededDisplay() const;
	int getAdjustedAlignment() const;
	void hasNewMudmail() const;
	bool checkConfusion();
	int autosplit(long amt);

	void courageous();
	void init();
	void uninit();
	void checkEffectsWearingOff();
	void update();
	void upLevel();
	void downLevel();
	void initLanguages();
	void setMonkDice();

	bool checkForSpam();
	void silenceSpammer();
	int getDeityClan() const;
	int chooseItem();
	//	int countResistSpells();
	void checkNumResistSpells();
	bool checkOppositeResistSpell(bstring effect);

	int computeLuck();
	int getArmorWeight() const;
	int getFallBonus();
	int getSneakChance();
	int getLight() const;
	int getVision() const;

	bool halftolevel() const;
	bool canWear(const Object* object, bool all=false) const;
	bool canUse(Object* object, bool all=false);
	void checkInventory();
	void checkTempEnchant(Object* object);
	void checkEnvenom(Object* object);
	bool breakObject(Object* object, int loc);
	void wearCursed();
	bool canChooseCustomTitle() const;
	bool alignInOrder() const;

	Socket* getSock() const;
	bool isConnected() const;
	void setSock(Socket* pSock);

	bool songIsKnown(int song) const;
	void learnSong(int song);
	void forgetSong(int song);

	bstring getPassword() const;
	bool isPassword(bstring pass) const;
	void setPassword(bstring pass);

	static bool exists(bstring name);
	time_t getIdle();
	int delCharm(Creature* creature);
	int addCharm(Creature* creature);

	void setLastPawn(Object* object);
	bool restoreLastPawn();
	void checkFreeSkills(bstring skill);
	void computeInterest(long t, bool online);

	void resetObjectIds();
	void setObjectId(Object* object);
};


#endif /*CREATURES_H_*/

/*
 * creatureAttr.cpp
 *   Creature attribute functions
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include "calendar.hpp"
#include "config.hpp"
#include "creatures.hpp"
#include "mud.hpp"
#include "raceData.hpp"
#include "rooms.hpp"
#include "specials.hpp"
#include "quests.hpp"
#include "version.hpp"
#include "xml.hpp"

//*********************************************************************
//                      getClass
//*********************************************************************

CreatureClass Creature::getClass() const { return(cClass); }

int Creature::getClassInt() const { return(static_cast<int>(cClass)); }

int Creature::getSecondClassInt() const {
    const Player* pThis = getAsConstPlayer();

    if(pThis)
        return(static_cast<int>(pThis->getSecondClass()));
    else
        return (static_cast<int>(CreatureClass::NONE));
}


//*********************************************************************
//                      getLevel
//*********************************************************************

unsigned short Creature::getLevel() const { return(level); }

//*********************************************************************
//                      getAlignment
//*********************************************************************

short Creature::getAlignment() const { return(alignment); }

//*********************************************************************
//                      getArmor
//*********************************************************************

unsigned int Creature::getArmor() const { return(armor); }

//*********************************************************************
//                      getExperience
//*********************************************************************

unsigned long Creature::getExperience() const { return(experience); }

//*********************************************************************
//                getCoinDisplay & getBankDisplay
//*********************************************************************
bstring Player::getCoinDisplay() const { return(coins.str()); }

bstring Player::getBankDisplay() const { return(bank.str()); }

//*********************************************************************
//                      getClan
//*********************************************************************

unsigned short Creature::getClan() const { return(clan); }

//*********************************************************************
//                      getType
//*********************************************************************

mType Creature::getType() const { return(type); }

//*********************************************************************
//                      getRace
//*********************************************************************

unsigned short Creature::getRace() const { return(race); }

//*********************************************************************
//                      getDeity
//*********************************************************************

unsigned short Creature::getDeity() const { return(deity); }

//*********************************************************************
//                      getSize
//*********************************************************************

Size Creature::getSize() const { return(size); }

//*********************************************************************
//                      getAttackPower
//*********************************************************************

unsigned int Creature::getAttackPower() const { return(attackPower); }

//*********************************************************************
//                      getDescription
//*********************************************************************

bstring Creature::getDescription() const { return(description); }

//*********************************************************************
//                      getVersion
//*********************************************************************

bstring Creature::getVersion() const { return(version); }

//*********************************************************************
//                      getPoisonDuration
//*********************************************************************

unsigned short Creature::getPoisonDuration() const { return(poison_dur); }

//*********************************************************************
//                      getPoisonDamage
//*********************************************************************

unsigned short Creature::getPoisonDamage() const { return(poison_dmg); }

//*********************************************************************
//                      getSex
//*********************************************************************

Sex Creature::getSex() const {
    if(isPlayer()) {
        if(flagIsSet(P_SEXLESS))
            return(SEX_NONE);
        if(flagIsSet(P_MALE))
            return(SEX_MALE);
        return(SEX_FEMALE);
    } else {
        if(flagIsSet(M_SEXLESS))
            return(SEX_NONE);
        if(flagIsSet(M_MALE))
            return(SEX_MALE);
        return(SEX_FEMALE);
    }
}

//*********************************************************************
//                      getRealm
//*********************************************************************

unsigned long Creature::getRealm(Realm r) const { return(realm[r-1]); }

//*********************************************************************
//                      getPoisonedBy
//*********************************************************************

bstring Creature::getPoisonedBy() const { return(poisonedBy); }

//*********************************************************************
//                      inJail
//*********************************************************************

bool Creature::inJail() const {
    if(isStaff())
        return(false);
    if(!inUniqueRoom())
        return(false);
    return(getConstUniqueRoomParent()->flagIsSet(R_JAIL));
}

//*********************************************************************
//                      addExperience
//*********************************************************************

void Creature::addExperience(unsigned long e) {
    setExperience(experience + e);
    if(isPlayer())
        getAsPlayer()->checkLevel();
}

//*********************************************************************
//                      subExperience
//*********************************************************************

void Creature::subExperience(unsigned long e) {
    setExperience(e > experience ? 0 : experience - e);
    int lost = (e > experience ? 0 : e);
    if(isPlayer()) {
        getAsPlayer()->checkLevel();
        getAsPlayer()->statistics.experienceLost(lost);
    }
}

//*********************************************************************
//                      setExperience
//*********************************************************************

void Creature::setExperience(unsigned long e) { experience = MIN<unsigned long>(2100000000, e); }

//*********************************************************************
//                      setClass
//*********************************************************************

void Creature::setClass(CreatureClass c) {
    if(cClass == CreatureClass::PUREBLOOD && c != CreatureClass::PUREBLOOD)
        removeEffect("vampirism");
    else if(cClass == CreatureClass::WEREWOLF && c != CreatureClass::WEREWOLF)
        removeEffect("lycanthropy");

    if(cClass != CreatureClass::PUREBLOOD && c == CreatureClass::PUREBLOOD) {
        if(!isEffected("vampirism")) {
            addPermEffect("vampirism");
            if(isPlayer())
                getAsPlayer()->makeVampire();
        }
    } else if(cClass != CreatureClass::WEREWOLF && c == CreatureClass::WEREWOLF) {
        if(!isEffected("lycanthropy")) {
            addPermEffect("lycanthropy");
            if(isPlayer())
                getAsPlayer()->makeWerewolf();
        }
    }
    if(c == CreatureClass::LICH) {
        // Liches don't get hp bonus for con
        constitution.setInfluences(nullptr);
        hp.setInfluencedBy(nullptr);
    }

    cClass = c;
}

//*********************************************************************
//                      setClan
//*********************************************************************

void Creature::setClan(unsigned short c) { clan = c; }

//*********************************************************************
//                      setLevel
//*********************************************************************

void Creature::setLevel(unsigned short l, bool isDm) { level = MAX(1, MIN<int>(l, isDm ? 127 : MAXALVL)); }

//*********************************************************************
//                      setAlignment
//*********************************************************************

void Creature::setAlignment(short a) { alignment = MAX<short>(-1000, MIN<short>(1000, a)); }

//*********************************************************************
//                      subAlignment
//*********************************************************************

void Creature::subAlignment(unsigned short a) { setAlignment(alignment - a); }

//*********************************************************************
//                      setArmor
//*********************************************************************

void Creature::setArmor(unsigned int a) { armor = MAX<unsigned int>(MIN(a, MAX_ARMOR), 0); }

//*********************************************************************
//                      setAttackPower
//*********************************************************************

void Creature::setAttackPower(unsigned int a) { attackPower = MIN<unsigned int>(1500, a); }

//*********************************************************************
//                      setDeity
//*********************************************************************

void Creature::setDeity(unsigned short d) { deity = MIN<unsigned short>(d, DEITY_COUNT-1); }

//*********************************************************************
//                      setRace
//*********************************************************************

void Creature::setRace(unsigned short r) { race = MIN<unsigned short>(gConfig->raceCount()-1, r); }

//*********************************************************************
//                      setSize
//*********************************************************************

void Creature::setSize(Size s) { size = MAX(NO_SIZE, MIN(MAX_SIZE, s)); }

//*********************************************************************
//                      setType
//*********************************************************************

void Creature::setType(unsigned short t) { type = (mType)MIN<short>(MAX_MOB_TYPES-1, t); }

//*********************************************************************
//                      setType
//*********************************************************************

void Creature::setType(mType t) { type = t; }

//*********************************************************************
//                      setDescription
//*********************************************************************

void Creature::setDescription(const bstring& desc) {
    description = desc;
    if(isMonster())
        description.Replace("*CR*", "\n");
}

//*********************************************************************
//                      setVersion
//*********************************************************************

void Creature::setVersion(bstring v) { version = v == "" ? VERSION : v; }

//*********************************************************************
//                      setVersion
//*********************************************************************

void Creature::setVersion(xmlNodePtr rootNode) { xml::copyPropToBString(version, rootNode, "Version"); }

//*********************************************************************
//                      setPoisonDuration
//*********************************************************************

void Creature::setPoisonDuration(unsigned short d) { poison_dur = d; }

//*********************************************************************
//                      setPoisonDamage
//*********************************************************************

void Creature::setPoisonDamage(unsigned short d) { poison_dmg = d; }

//*********************************************************************
//                      setSex
//*********************************************************************

void Creature::setSex(Sex sex) {
    if(isPlayer()) {
        if(sex == SEX_FEMALE) {
            clearFlag(P_SEXLESS);
            clearFlag(P_MALE);
        } else if(sex == SEX_MALE) {
            clearFlag(P_SEXLESS);
            setFlag(P_MALE);
        } else {
            setFlag(P_SEXLESS);
            clearFlag(P_MALE);
        }
    } else {
        if(sex == SEX_FEMALE) {
            clearFlag(M_SEXLESS);
            clearFlag(M_MALE);
        } else if(sex == SEX_MALE) {
            clearFlag(M_SEXLESS);
            setFlag(M_MALE);
        } else {
            setFlag(M_SEXLESS);
            clearFlag(M_MALE);
        }
    }
}

//*********************************************************************
//                      getSexName
//*********************************************************************

bstring getSexName(Sex sex) {
    if(sex == SEX_FEMALE)
        return("Female");
    if(sex == SEX_MALE)
        return("Male");
    if(sex == SEX_NONE)
        return("Genderless");
    return("Unknown");
}

//*********************************************************************
//                      setDeathType
//*********************************************************************

void Creature::setDeathType(DeathType d) { deathtype = d; }

//*********************************************************************
//                      setRealm
//*********************************************************************

void Creature::setRealm(unsigned long num, Realm r) { realm[r-1] = MIN<unsigned long>(10000000, num); }

//*********************************************************************
//                      addRealm
//*********************************************************************

void Creature::addRealm(unsigned long num, Realm r) { setRealm(getRealm(r) + num, r); }

//*********************************************************************
//                      subRealm
//*********************************************************************

void Creature::subRealm(unsigned long num, Realm r) { setRealm(num > getRealm(r) ? 0 : getRealm(r) - num, r); }

//*********************************************************************
//                      setPoisonedBy
//*********************************************************************

void Creature::setPoisonedBy(const bstring& p) { poisonedBy = p; }

//*********************************************************************
//                      getMobTrade
//*********************************************************************

unsigned short Monster::getMobTrade() const { return(mobTrade); }

//*********************************************************************
//                      getSkillLevel
//*********************************************************************

int Monster::getSkillLevel() const { return(skillLevel); }

//*********************************************************************
//                      getMaxLevel
//*********************************************************************

unsigned int Monster::getMaxLevel() const { return(maxLevel); }

//*********************************************************************
//                      getNumWander
//*********************************************************************

unsigned short Monster::getNumWander() const { return(numwander); }

//*********************************************************************
//                      getLoadAggro
//*********************************************************************

unsigned short Monster::getLoadAggro() const { return(loadAggro); }

//*********************************************************************
//                      getUpdateAggro
//*********************************************************************

unsigned short Monster::getUpdateAggro() const { return(updateAggro); }

//*********************************************************************
//                      getCastChance
//*********************************************************************

unsigned short Monster::getCastChance() const { return(cast); }

//*********************************************************************
//                      getMagicResistance
//*********************************************************************

unsigned short Monster::getMagicResistance() const { return(magicResistance); }

//*********************************************************************
//                      getPrimeFaction
//*********************************************************************

bstring Monster::getPrimeFaction() const { return(primeFaction); }

//*********************************************************************
//                      getTalk
//*********************************************************************

bstring Monster::getTalk() const { return(talk); }

//*********************************************************************
//                      setMaxLevel
//*********************************************************************

void Monster::setMaxLevel(unsigned short l) { maxLevel = MAX<unsigned short>(0, MIN<unsigned short>(l, MAXALVL)); }

//*********************************************************************
//                      setCastChance
//*********************************************************************

void Monster::setCastChance(unsigned short c) { cast = MAX<unsigned short>(0, MIN<unsigned short>(c, 100)); }

//*********************************************************************
//                      setMagicResistance
//*********************************************************************

void Monster::setMagicResistance(unsigned short m) { magicResistance = MAX<unsigned short>(0, MIN<unsigned short>(100, m)); }

//*********************************************************************
//                      setLoadAggro
//*********************************************************************

void Monster::setLoadAggro(unsigned short a) { loadAggro = MAX<unsigned short>(0, MIN<unsigned short>(a, 99)); }

//*********************************************************************
//                      setUpdateAggro
//*********************************************************************

void Monster::setUpdateAggro(unsigned short a) { updateAggro = MAX<unsigned short>(1, MIN<unsigned short>(a, 99)); }

//*********************************************************************
//                      setNumWander
//*********************************************************************

void Monster::setNumWander(unsigned short n) { numwander = MAX<unsigned short>(0, MIN<unsigned short>(6, n)); }

//*********************************************************************
//                      setSkillLevel
//*********************************************************************

void Monster::setSkillLevel(int l) { skillLevel = MAX(0, MIN(100, l)); }

//*********************************************************************
//                      setMobTrade
//*********************************************************************

void Monster::setMobTrade(unsigned short t) { mobTrade = MAX<unsigned short>(0,MIN<unsigned short>(MOBTRADE_COUNT-1, t)); }

//*********************************************************************
//                      setPrimeFaction
//*********************************************************************

void Monster::setPrimeFaction(bstring f) { primeFaction = f; }

//*********************************************************************
//                      setTalk
//*********************************************************************

void Monster::setTalk(bstring t) { talk = t; talk.Replace("*CR*", "\n"); }

//*********************************************************************
//                      getSecondClass
//*********************************************************************

CreatureClass Player::getSecondClass() const { return(cClass2); }
bool Player::hasSecondClass() const { return(cClass2 != CreatureClass::NONE); }

//*********************************************************************
//                      getGuild
//*********************************************************************

unsigned short Player::getGuild() const { return(guild); }

//*********************************************************************
//                      getGuildRank
//*********************************************************************

unsigned short Player::getGuildRank() const { return(guildRank); }

//*********************************************************************
//                      getActualLevel
//*********************************************************************

unsigned short Player::getActualLevel() const { return(actual_level); }

//*********************************************************************
//                      getNegativeLevels
//*********************************************************************

unsigned short Player::getNegativeLevels() const { return(negativeLevels); }

//*********************************************************************
//                      getWimpy
//*********************************************************************

unsigned short Player::getWimpy() const { return(wimpy); }

//*********************************************************************
//                      getTickDamage
//*********************************************************************

unsigned short Player::getTickDamage() const { return(tickDmg); }

//*********************************************************************
//                      getWarnings
//*********************************************************************

unsigned short Player::getWarnings() const { return(warnings); }

//*********************************************************************
//                      getPkin
//*********************************************************************

unsigned short Player::getPkin() const { return(pkin); }

//*********************************************************************
//                      getPkwon
//*********************************************************************

unsigned short Player::getPkwon() const { return(pkwon); }

//*********************************************************************
//                      getWrap
//*********************************************************************

int Player::getWrap() const { return(wrap); }

//*********************************************************************
//                      getLuck
//*********************************************************************

short Player::getLuck() const { return(luck); }

//*********************************************************************
//                      getWeaponTrains
//*********************************************************************

unsigned short Player::getWeaponTrains() const { return(weaponTrains); }

//*********************************************************************
//                      getLastLogin
//*********************************************************************

long Player::getLastLogin() const { return(lastLogin); }

//*********************************************************************
//                      getLastInterest
//*********************************************************************

long Player::getLastInterest() const { return(lastInterest); }

//*********************************************************************
//                      getLastPassword
//*********************************************************************

bstring Player::getLastPassword() const { return(lastPassword); }

//*********************************************************************
//                      getAfflictedBy
//*********************************************************************

bstring Player::getAfflictedBy() const { return(afflictedBy); }

//*********************************************************************
//                      getLastCommunicate
//*********************************************************************

bstring Player::getLastCommunicate() const { return(lastCommunicate); }

//*********************************************************************
//                      getLastCommand
//*********************************************************************

bstring Player::getLastCommand() const { return(lastCommand); }

//*********************************************************************
//                      getSurname
//*********************************************************************

bstring Player::getSurname() const { return(surname); }

//*********************************************************************
//                      getForum
//*********************************************************************

bstring Player::getForum() const { return(forum); }

//*********************************************************************
//                      getCreated
//*********************************************************************

long Player::getCreated() const { return(created); }

//*********************************************************************
//                      getCreatedStr
//*********************************************************************

bstring Player::getCreatedStr() const {
    bstring str;
    if(created) {
        str = ctime(&created);
        str.trim();
    } else
        str = oldCreated;
    return(str);
}

//*********************************************************************
//                      getAlias
//*********************************************************************

Monster* Player::getAlias() const { return(alias_crt); }

//*********************************************************************
//                      getBirthday
//*********************************************************************

cDay* Player::getBirthday() const { return(birthday); }

//*********************************************************************
//                      getAnchorAlias
//*********************************************************************

bstring Player::getAnchorAlias(int i) const { return(anchor[i] ? anchor[i]->getAlias() : ""); }

//*********************************************************************
//                      getAnchorRoomName
//*********************************************************************

bstring Player::getAnchorRoomName(int i) const { return(anchor[i] ? anchor[i]->getRoomName() : ""); }

//*********************************************************************
//                      getAnchor
//*********************************************************************

const Anchor* Player::getAnchor(int i) const { return(anchor[i]); }

//*********************************************************************
//                      hasAnchor
//*********************************************************************

bool Player::hasAnchor(int i) const { return(!!anchor[i]); }

//*********************************************************************
//                      isAnchor
//*********************************************************************

bool Player::isAnchor(int i, const BaseRoom* room) const { return(anchor[i]->is(room)); }

//*********************************************************************
//                      getThirst
//*********************************************************************

unsigned short Player::getThirst() const { return(thirst); }

//*********************************************************************
//                      numDiscoveredRooms
//*********************************************************************

int Player::numDiscoveredRooms() const { return(roomExp.size()); }

//*********************************************************************
//                      getUniqueObjId
//*********************************************************************

int Player::getUniqueObjId() const { return(uniqueObjId); }

//*********************************************************************
//                      setTickDamage
//*********************************************************************

void Player::setTickDamage(unsigned short t) { tickDmg = t; }

//*********************************************************************
//                      setWarnings
//*********************************************************************

void Player::setWarnings(unsigned short w) { warnings = w; }

//*********************************************************************
//                      addWarnings
//*********************************************************************

void Player::addWarnings(unsigned short w) { setWarnings(w + warnings); }

//*********************************************************************
//                      subWarnings
//*********************************************************************

void Player::subWarnings(unsigned short w) { setWarnings(w > warnings ? 0 : warnings - w); }

//*********************************************************************
//                      setWimpy
//*********************************************************************

void Player::setWimpy(unsigned short w) { wimpy = w; }

//*********************************************************************
//                      setActualLevel
//*********************************************************************

void Player::setActualLevel(unsigned short l) { actual_level = MAX<unsigned short>(1, MIN<unsigned short>(l, MAXALVL)); }

//*********************************************************************
//                      setSecondClass
//*********************************************************************

void Player::setSecondClass(CreatureClass c) { cClass2 = c; }

//*********************************************************************
//                      setGuild
//*********************************************************************

void Player::setGuild(unsigned short g) { guild = g; }

//*********************************************************************
//                      setGuildRank
//*********************************************************************

void Player::setGuildRank(unsigned short g) { guildRank = g; }

//*********************************************************************
//                      setNegativeLevels
//*********************************************************************

void Player::setNegativeLevels(unsigned short l) { negativeLevels = MAX<unsigned short>(0, MIN<unsigned short>(exp_to_lev(experience), l)); }

//*********************************************************************
//                      setLuck
//*********************************************************************

void Player::setLuck(int l) { luck = l; }

//*********************************************************************
//                      setWeaponTrains
//*********************************************************************

void Player::setWeaponTrains(unsigned short t) { weaponTrains = t; }

//*********************************************************************
//                      subWeaponTrains
//*********************************************************************

void Player::subWeaponTrains(unsigned short t) { setWeaponTrains(t > weaponTrains ? 0 : weaponTrains - t); }

//*********************************************************************
//                      setLastPassword
//*********************************************************************

void Player::setLastPassword(bstring p) { lastPassword = p; }

//*********************************************************************
//                      setAfflictedBy
//*********************************************************************

void Player::setAfflictedBy(bstring a) { afflictedBy = a; }

//*********************************************************************
//                      setLastLogin
//*********************************************************************

void Player::setLastLogin(long l) { lastLogin = MAX<long>(0, l); }

//*********************************************************************
//                      setLastInterest
//*********************************************************************

void Player::setLastInterest(long l) { lastInterest = MAX<long>(0, l); }

//*********************************************************************
//                      setLastCommunicate
//*********************************************************************

void Player::setLastCommunicate(bstring c) { lastCommunicate = c; }

//*********************************************************************
//                      setLastCommand
//*********************************************************************

void Player::setLastCommand(bstring c) { lastCommand = c; lastCommand.trim(); }

//*********************************************************************
//                      setCreated
//*********************************************************************

void Player::setCreated() { created = time(0); }

//*********************************************************************
//                      setSurname
//*********************************************************************

void Player::setSurname(bstring s) { surname = s.left(20); }

//*********************************************************************
//                      setForum
//*********************************************************************

void Player::setForum(bstring f) { forum = f; }

//*********************************************************************
//                      setAlias
//*********************************************************************

void Player::setAlias(Monster* m) { alias_crt = m; }

//*********************************************************************
//                      setBirthday
//*********************************************************************

void Player::setBirthday() {
    const Calendar* calendar = gConfig->getCalendar();
    if(birthday)
        delete birthday;
    birthday = new cDay;
    birthday->setYear(calendar->getCurYear() - gConfig->getRace(getRace())->getStartAge());
    birthday->setDay(calendar->getCurDay());
    birthday->setMonth(calendar->getCurMonth());
}

//*********************************************************************
//                      delAnchor
//*********************************************************************

void Player::delAnchor(int i) {
    if(anchor[i]) {
        delete anchor[i];
        anchor[i] = 0;
    }
}

//*********************************************************************
//                      setAnchor
//*********************************************************************

void Player::setAnchor(int i, bstring a) {
    delAnchor(i);
    anchor[i] = new Anchor(a, this);
}

//*********************************************************************
//                      setThirst
//*********************************************************************

void Player::setThirst(unsigned short t) { thirst = t; }

int Player::setWrap(int newWrap) {
    if(newWrap < -1)
        newWrap = -1;
    if(newWrap > 500)
        newWrap = 500;

    wrap = newWrap;
    return(wrap);
}


//*********************************************************************
//                      setCustomColor
//*********************************************************************

void Player::setCustomColor(CustomColor i, char c) { customColors[i] = c; }

//*********************************************************************
//                      getMaster
//*********************************************************************

Creature* Creature::getMaster() {
    return((isMonster() ? getAsMonster()->getMaster() : this));
}

//*********************************************************************
//                      getConstMaster
//*********************************************************************

const Creature* Creature::getConstMaster() const {
    return((isMonster() ? getAsConstMonster()->getMaster() : this));
}
//*********************************************************************
//                      getPlayerMaster
//*********************************************************************

Player* Creature::getPlayerMaster() {
    if(!getMaster())
        return(nullptr);
    return(getMaster()->getAsPlayer());
}

//*********************************************************************
//                      getConstPlayerMaster
//*********************************************************************

const Player* Creature::getConstPlayerMaster() const {
    if(!getConstMaster())
        return(nullptr);
    return(getConstMaster()->getAsConstPlayer());
}


//*********************************************************************
//                      isPlayer
//*********************************************************************

bool Creature::isPlayer() const {
    return(type == PLAYER);
}

//*********************************************************************
//                      isMonster
//*********************************************************************

bool Creature::isMonster() const {
    return(type != PLAYER);
}

//*********************************************************************
//                      crtReset
//*********************************************************************

// Resets all common structure elements of creature
void Creature::crtReset() {
    // Reset Stream related
    initStreamable();

    // Reset other related
    moReset();
    playing = nullptr;
    myTarget = nullptr;

    for(Creature* targeter : targetingThis) {
        targeter->clearTarget(false);
    }

    // Clear out any skills/factions/etc
    crtDestroy();

    zero(key, sizeof(key));

    poisonedBy = description = "";
    version = "0.00";

    fd = -1;
    cClass = CreatureClass::NONE;
    level = race = alignment = experience =
        temp_experience = clan = 0;
    size = NO_SIZE;
    type = PLAYER;
    deathtype = DT_NONE;

    int i;
    coins.zero();
    zero(realm, sizeof(realm));
    zero(flags, sizeof(flags));
    zero(spells, sizeof(spells));
    zero(old_quests, sizeof(old_quests));
    zero(languages, sizeof(languages));
    questnum = 0;

    strength.setName("Strength");
    dexterity.setName("Dexterity");
    constitution.setName("Constitution");
    intelligence.setName("Intelligence");
    piety.setName("Piety");

    hp.setName("Hp");
    mp.setName("Mp");

    if(getAsPlayer()) {
        getAsPlayer()->focus.setName("Focus");

        constitution.setInfluences(&hp);
        hp.setInfluencedBy(&constitution);

        intelligence.setInfluences(&mp);
        mp.setInfluencedBy(&intelligence);
    }

    for(i=0; i<MAXWEAR; i++)
        ready[i] = 0;

    first_tlk = 0;

    currentLocation.mapmarker.reset();
    currentLocation.room.clear();

    group = 0;
    groupStatus = GROUP_NO_STATUS;

    current_language = 0;
    afterProf = 0;
    memset(movetype, 0, sizeof(movetype));
    poison_dur = poison_dmg = 0;

    for(i=0; i<6; i++)
        proficiency[i] = 0;
    deity = 0;

    memset(spellTimer, 0, sizeof(spellTimer));
    armor = 0;

    for(i=0; i<21; i++)
        misc[i] = 0;
}

//*********************************************************************
//                      reset
//*********************************************************************

void Monster::reset() {
    // Call Creature::reset first
    crtReset();
    int i;

    info.clear();
    memset(attack, 0, sizeof(attack));
    memset(aggroString, 0, sizeof(aggroString));
    talk = "";
    plural = "";

    baseRealm = NO_REALM;
    defenseSkill = weaponSkill = attackPower = 0;

    numwander = loadAggro = 0;
    memset(last_mod, 0, sizeof(last_mod));

    mobTrade = 0;
    skillLevel = 0;
    maxLevel = 0;
    memset(ttalk, 0, sizeof(ttalk));

    for(i=0; i<NUM_RESCUE; i++)
        rescue[i].clear();

    myMaster = 0;
    updateAggro = 0;
    cast = 0;
    magicResistance = 0;
    jail.clear();

    primeFaction = "";
    memset(cClassAggro, 0, sizeof(cClassAggro));
    memset(raceAggro, 0, sizeof(raceAggro));
    memset(deityAggro, 0, sizeof(deityAggro));

    std::list<TalkResponse*>::iterator tIt;
    for(tIt = responses.begin() ; tIt != responses.end() ; tIt++) {
        delete (*tIt);
    }
    responses.clear();
}

//*********************************************************************
//                      reset
//*********************************************************************

void Player::reset() {
    // Call Creature::reset first
    crtReset();
    //playing = NULL;
    wrap = -1;
    cClass2 = CreatureClass::NONE;
    wimpy = 0;
    barkskin = 0;
    weaponTrains = 0;
    bank.zero();
    created = 0;

    oldCreated = surname = lastCommand = lastCommunicate = password = title = tempTitle = "";
    lastPassword = afflictedBy = forum = "";
    tickDmg = pkwon = pkin = lastLogin = lastInterest = uniqueObjId = 0;

    memset(songs, 0, sizeof(songs));
    guild = guildRank = 0;
    cClass2 = CreatureClass::NONE;

    int i;
    actual_level = warnings = 0;
    for(i=0; i<MAX_DIMEN_ANCHORS; i++)
        anchor[i] = 0;

    negativeLevels = 0;
    birthday = nullptr;
    first_charm = 0;

    luck = 0;
    ansi = 0;
    alias_crt = 0;
    scared_of = 0;
    for(i=0; i<5; i++)
        tnum[i] = 0;
    timeout = 0;
    thirst = 0;

    resetCustomColors();
    lastPawn = 0;

    storesRefunded.clear();
    roomExp.clear();
    objIncrease.clear();
    lore.clear();
    recipes.clear();
    for(auto qIt = questsInProgress.begin() ; qIt != questsInProgress.end() ; qIt++) {
        delete (*qIt).second;
    }
    questsInProgress.clear();
    for(auto qIt = questsCompleted.begin() ; qIt != questsCompleted.end() ; qIt++) {
        delete (*qIt).second;
    }
    questsCompleted.clear();
}

//*********************************************************************
//                      Creature
//*********************************************************************

Creature::Creature() {
    hooks.setParent(this);
}

//*********************************************************************
//                      CopyCommon
//*********************************************************************

void Creature::CopyCommon(const Creature& cr) {
    int     i=0;

    moCopy(cr);

    description = cr.description;
    for(i=0; i<3; i++) {
        strcpy(key[i], cr.key[i]);
    }
    fd = cr.fd;
    level = cr.level;
    type = cr.type;
    cClass = cr.cClass;
    race = cr.race;
    alignment = cr.alignment;
    experience = cr.experience;
    temp_experience = cr.temp_experience;
    poisonedBy = cr.poisonedBy;

    damage = cr.damage;

    clan = cr.clan;
    questnum = cr.questnum;
    size = cr.size;

    for(Realm r = MIN_REALM; r<MAX_REALM; r = (Realm)((int)r + 1))
        setRealm(cr.getRealm(r), r);

    for(i=0; i<CRT_FLAG_ARRAY_SIZE; i++)
        flags[i] = cr.flags[i];

    poison_dur = cr.poison_dur;
    poison_dmg = cr.poison_dmg;

    strength = cr.strength;
    dexterity = cr.dexterity;
    constitution = cr.constitution;
    intelligence = cr.intelligence;
    piety = cr.piety;

    hp = cr.hp;
    mp = cr.mp;
    pp = cr.pp;
    rp = cr.rp;

    hpTic = cr.hpTic;
    mpTic = cr.mpTic;
    ppTic = cr.ppTic;
    rpTic = cr.rpTic;

    armor = cr.armor;
    current_language = cr.current_language;

    deity = cr.deity;

    for(i=0; i<MAXWEAR; i++)
        ready[i] = cr.ready[i];

    //following = cr.following;

    group = cr.group;
    groupStatus = cr.groupStatus;
    first_tlk = cr.first_tlk;
    objects = cr.objects;

    currentLocation.room = cr.currentLocation.room;
    currentLocation.mapmarker = cr.currentLocation.mapmarker;

    parent = cr.parent;

    for(i=0; i<6; i++)
        saves[i] = cr.saves[i];
    for(i=0; i<16; i++)
        languages[i] = cr.languages[i];
    for(i=0; i<32; i++) {
        spells[i] = cr.spells[i];
        old_quests[i] = cr.old_quests[i];
    }

    coins.set(cr.coins);

    for(i=0; i<6; i++)
        proficiency[i] = cr.proficiency[i];
    for(i=0; i<20; i++)
        daily[i] = cr.daily[i];
    for(i=0; i<128; i++)
        lasttime[i] = cr.lasttime[i];
    for(i=0; i<16; i++)
        spellTimer[i] = cr.spellTimer[i];

    factions = cr.factions;

    //skills = cr.skills;
    Skill* skill;
    Skill* crSkill;
    std::map<bstring, Skill*>::const_iterator csIt;
    for(csIt = cr.skills.begin() ; csIt != cr.skills.end() ; csIt++) {
        crSkill = (*csIt).second;
        skill = new Skill;
        (*skill) = (*crSkill);
        skills[crSkill->getName()] = skill;
    }

    effects.copy(&cr.effects, this);

    std::list<bstring>::const_iterator mIt;
    for(mIt = cr.minions.begin() ; mIt != cr.minions.end() ; mIt++) {
        minions.push_back(*mIt);
    }

    SpecialAttack* attack;
    std::list<SpecialAttack*>::const_iterator sIt;
    for(sIt = cr.specials.begin() ; sIt != cr.specials.end() ; sIt++) {
        attack = new SpecialAttack();
        (*attack) = *(*sIt);
        specials.push_back(attack);
    }

    for(Monster* pet : cr.pets) {
        pets.push_back(pet);
    }


    attackPower = cr.attackPower;
    previousRoom = cr.previousRoom;
}

//*********************************************************************
//                      doCopy
//*********************************************************************

void Player::doCopy(const Player& cr) {
    // We want a copy of what we're getting, so clear out anything that was here before
    reset();
    // Copy anything in common with the base class
    CopyCommon(cr);

    // Players have a unique ID, so copy that
    id = cr.id;

    // Now copy player specfic stuff
    int i=0;
    created = cr.created;
    oldCreated = cr.oldCreated;

    playing = cr.playing;

    wrap = cr.wrap;

    title = cr.title;
    password = cr.getPassword();
    surname = cr.surname;
    forum = cr.forum;

    bank.set(cr.bank);

    lastPassword = cr.lastPassword;
    lastCommand = cr.lastCommand;
    lastCommunicate = cr.lastCommunicate;
    afflictedBy = cr.afflictedBy;
    lastLogin = cr.lastLogin;
    lastInterest = cr.lastInterest;
    tempTitle = cr.tempTitle;

    std::list<CatRef>::const_iterator it;

    if(!cr.storesRefunded.empty()) {
        for(it = cr.storesRefunded.begin() ; it != cr.storesRefunded.end() ; it++) {
            storesRefunded.push_back(*it);
        }
    }

    if(!cr.roomExp.empty()) {
        for(it = cr.roomExp.begin() ; it != cr.roomExp.end() ; it++) {
            roomExp.push_back(*it);
        }
    }
    if(!cr.objIncrease.empty()) {
        for(it = cr.objIncrease.begin() ; it != cr.objIncrease.end() ; it++) {
            objIncrease.push_back(*it);
        }
    }
    if(!cr.lore.empty()) {
        for(it = cr.lore.begin() ; it != cr.lore.end() ; it++) {
            lore.push_back(*it);
        }
    }

    std::list<int>::const_iterator rt;
    if(!cr.recipes.empty()) {
        for(rt = cr.recipes.begin() ; rt != cr.recipes.end() ; rt++) {
            recipes.push_back(*rt);
        }
    }

    cClass2 = cr.cClass2;
    wimpy = cr.wimpy;
    tickDmg = cr.tickDmg;
    pkwon = cr.pkwon;
    pkin = cr.pkin;
    bound = cr.bound;
    statistics = cr.statistics;

    for(i=0; i<32; i++)
        songs[i] = cr.songs[i];

    guild = cr.guild;
    guildRank = cr.guildRank;
    cClass2 = cr.cClass2;
    focus = cr.focus;

    negativeLevels = cr.negativeLevels;
    actual_level = cr.actual_level;
    warnings = cr.warnings;

    for(i=0; i<3; i++)
        strcpy(movetype[i], cr.movetype[i]);

    for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
        if(cr.anchor[i]) {
            anchor[i] = new Anchor;
            *anchor[i] = *cr.anchor[i];
        }
    }

    for(i=0; i<MAX_BUILDER_RANGE; i++) {
        bRange[i] = cr.bRange[i];
    }

    for(i=0; i<21; i++)
        misc[i] = cr.misc[i];

    if(cr.birthday) {
        birthday = new cDay;
        *(birthday) = *(cr.birthday);
    }

    strcpy(customColors, cr.customColors);
    thirst = cr.thirst;
    weaponTrains = cr.weaponTrains;
    attackTimer = cr.attackTimer;

    for(auto p : cr.questsInProgress) {
        questsInProgress[p.first] = new QuestCompletion(*(p.second));
    }

    for(auto qc : cr.questsCompleted) {
        questsCompleted.insert(std::make_pair(qc.first, new QuestCompleted(*qc.second)));
    }

    for(auto& p : cr.knownAlchemyEffects) {
        knownAlchemyEffects.insert(p);
    }
}

//*********************************************************************
//                      doCopy
//*********************************************************************

void Monster::doCopy(const Monster& cr) {
    // We want a copy of what we're getting, so clear out anything that was here before
    reset();
    // Copy anything in common with the base class
    CopyCommon(cr);

    // Now do monster specific copies
    int i;
    strcpy(aggroString, cr.aggroString);
    talk = cr.talk;
    plural = cr.plural;

    strcpy(last_mod, cr.last_mod);
    strcpy(ttalk, cr.ttalk);

    for(i=0; i<3; i++)
        strcpy(attack[i], cr.attack[i]);
    for(i=0; i<NUM_RESCUE; i++)
        rescue[i] = cr.rescue[i];

    baseRealm = cr.baseRealm;

    mobTrade = cr.mobTrade;
    skillLevel = cr.skillLevel;
    maxLevel = cr.maxLevel;
    jail = cr.jail;
    cast = cr.cast;

    myMaster = cr.myMaster;

    loadAggro = cr.loadAggro;
    info = cr.info;

    updateAggro = cr.updateAggro;
    numwander = cr.numwander;

    for(i=0; i<10; i++)
        carry[i] = cr.carry[i];

    primeFaction = cr.primeFaction;
    magicResistance = cr.magicResistance;


    for(i=0; i<NUM_ASSIST_MOB; i++)
        assist_mob[i] = cr.assist_mob[i];
    for(i=0; i<NUM_ENEMY_MOB; i++)
        enemy_mob[i] = cr.enemy_mob[i];

    for(i=0; i<3; i++) {
        strcpy(movetype[i], cr.movetype[i]);
    }

    for(i=0; i<4; i++) {
        cClassAggro[i] = cr.cClassAggro[i];
        raceAggro[i] = cr.raceAggro[i];
        deityAggro[i] = cr.deityAggro[i];
    }

    for(i=0; i<21; i++)
        misc[i] = cr.misc[i];


    defenseSkill = cr.defenseSkill;
    weaponSkill = cr.weaponSkill;

    for(TalkResponse* response : cr.responses) {
        responses.push_back(new TalkResponse(*response));
    }
    for(QuestInfo* quest : cr.quests) {
        quests.push_back(quest);
    }
}

//*********************************************************************
//                      getLocation
//*********************************************************************

Monster::Monster() {
    reset();
    threatTable = new ThreatTable(this);
}

Monster::Monster(Monster& cr) {
    doCopy(cr);
}

Monster::Monster(const Monster& cr) {
    doCopy(cr);
}

//*********************************************************************
//                      Monster operators
//*********************************************************************

Monster& Monster::operator=(const Monster& cr) {
    doCopy(cr);
    return(*this);
}

//*********************************************************************
//                      Player
//*********************************************************************

Player::Player() {
    reset();
    mySock = nullptr;
    // initial flags for new characters
    setFlag(P_NO_AUTO_WEAR);
}

Player::Player(Player& cr) {
    doCopy(cr);
}

Player::Player(const Player& cr) {
    doCopy(cr);
}

//*********************************************************************
//                      Player operators
//*********************************************************************

Player& Player::operator=(const Player& cr) {
    doCopy(cr);
    return(*this);
}

//*********************************************************************
//                      crtDestroy
//*********************************************************************

// Things all subclasses must destroy
void Creature::crtDestroy() {

    for(Creature* targeter : targetingThis) {
        targeter->clearTarget(false);
    }

    clearTarget();

    moDestroy();

    factions.clear();

    Skill* skill;
    std::map<bstring, Skill*>::iterator csIt;
    for(csIt = skills.begin() ; csIt != skills.end() ; csIt++) {
        skill = (*csIt).second;
        delete skill;
    }
    skills.clear();

    effects.removeAll();
    minions.clear();

    SpecialAttack* attack;
    std::list<SpecialAttack*>::iterator sIt;
    for(sIt = specials.begin() ; sIt != specials.end() ; sIt++) {
        attack = (*sIt);
        delete attack;
        (*sIt) = nullptr;
    }
    specials.clear();
}

//*********************************************************************
//                      Monster
//*********************************************************************

Monster::~Monster() {
    crtDestroy();
    for(auto it = responses.begin(); it != responses.end();) {
    	auto response = (*it);
    	it++;
    	delete response;
    }
    responses.clear();

    if(threatTable) {
        delete threatTable;
        threatTable = 0;
    }
}

//*********************************************************************
//                      Player
//*********************************************************************

Player::~Player() {
    crtDestroy();
    int i = 0;

    if(birthday) {
        delete birthday;
        birthday = nullptr;
    }

    for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
        if(anchor[i]) {
            delete anchor[i];
            anchor[i] = 0;
        }
    }

    for(auto qp : questsInProgress) {
        delete qp.second;
    }
    for(auto qc : questsCompleted) {
        delete qc.second;
    }

    questsInProgress.clear();
    questsCompleted.clear();
    setLastPawn(0);
}

//*********************************************************************
//                      isPublicWatcher
//*********************************************************************

bool Creature::isPublicWatcher() const {
    if(isMonster())
        return(false);
    return(flagIsSet(P_WATCHER) && !flagIsSet(P_SECRET_WATCHER));
}

//*********************************************************************
//                      isWatcher
//*********************************************************************

bool Creature::isWatcher() const {
    if(isMonster())
        return(false);
    return(isCt() || flagIsSet(P_WATCHER));
}

//*********************************************************************
//                      isStaff
//*********************************************************************

bool Creature::isStaff() const {
    if(isMonster())
        return(false);
    return(cClass >= CreatureClass::BUILDER);
}

//*********************************************************************
//                      isCt
//*********************************************************************

bool Creature::isCt() const {
    if(isMonster())
        return(false);
    return(cClass >= CreatureClass::CARETAKER);
}

//*********************************************************************
//                      isDm
//*********************************************************************

bool Creature::isDm() const {
    if(isMonster())
        return(false);
    return(cClass == CreatureClass::DUNGEONMASTER);
}

//*********************************************************************
//                      isAdm
//*********************************************************************

bool Creature::isAdm() const {
    if(isMonster())
        return(false);
    return(getName() == "Bane" || getName() == "Dominus" || getName() == "Ocelot");
}

//*********************************************************************
//                      flagIsSet
//*********************************************************************

bool Creature::flagIsSet(int flag) const {
    return(flags[flag/8] & 1<<(flag%8));
}
bool Creature::pFlagIsSet(int flag) const {
    return(isPlayer() && flagIsSet(flag));
}
bool Creature::mFlagIsSet(int flag) const {
    return(isMonster() && flagIsSet(flag));
}
//*********************************************************************
//                      setFlag
//*********************************************************************

void Creature::setFlag(int flag) {
    flags[flag/8] |= 1<<(flag%8);
    if(flag == P_NO_TRACK_STATS && isPlayer())
        getAsPlayer()->statistics.track = false;
}
void Creature::pSetFlag(int flag) {
    if(!isPlayer()) return;
    setFlag(flag);
}
void Creature::mSetFlag(int flag) {
    if(!isMonster()) return;
    setFlag(flag);
}


//*********************************************************************
//                      clearFlag
//*********************************************************************

void Creature::clearFlag(int flag) {
    flags[flag/8] &= ~(1<<(flag%8));
    if(flag == P_NO_TRACK_STATS && isPlayer())
        getAsPlayer()->statistics.track = true;
}

void Creature::pClearFlag(int flag) {
    if(!isPlayer()) return;
    clearFlag(flag);
}

void Creature::mClearFlag(int flag) {
    if(!isMonster()) return;
    clearFlag(flag);
}


//*********************************************************************
//                      toggleFlag
//*********************************************************************

bool Creature::toggleFlag(int flag) {
    if(flagIsSet(flag))
        clearFlag(flag);
    else
        setFlag(flag);
    return(flagIsSet(flag));
}

//*********************************************************************
//                      languageIsKnown
//*********************************************************************

bool Creature::languageIsKnown(int lang) const {
    return(languages[lang/8] & 1<<(lang%8));
}

//*********************************************************************
//                      learnLanguage
//*********************************************************************

void Creature::learnLanguage(int lang) {
    languages[lang/8] |= 1<<(lang%8);
}

//*********************************************************************
//                      forgetLanguage
//*********************************************************************

void Creature::forgetLanguage(int lang) {
    languages[lang/8] &= ~(1<<(lang%8));
}

//*********************************************************************
//                      spellIsKnown
//*********************************************************************

bool Creature::spellIsKnown(int spell) const {
    return(spells[spell/8] & 1<<(spell%8));
}

//*********************************************************************
//                      learnSpell
//*********************************************************************

extern int spllist_size;

void Creature::learnSpell(int spell) {
    if(spell > spllist_size) {
        broadcast(::isDm, "^G*** Trying to set invalid spell %d on %s.  Spell List Size: %d\n", spell, getCName(), spllist_size);
        return;
    }
    spells[spell/8] |= 1<<(spell%8);
}

//*********************************************************************
//                      forgetSpell
//*********************************************************************

void Creature::forgetSpell(int spell) {
    spells[spell/8] &= ~(1<<(spell%8));
}

//*********************************************************************
//                      questIsSet
//*********************************************************************

bool Player::questIsSet(int quest) const {
    return(old_quests[quest/8] & 1<<(quest%8));
}

//*********************************************************************
//                      setQuest
//*********************************************************************

void Player::setQuest(int quest) {
    old_quests[quest/8] |= 1<<(quest%8);
}

//*********************************************************************
//                      clearQuest
//*********************************************************************

void Player::clearQuest(int quest) {
    old_quests[quest/8] &= ~(1<<(quest%8));
}

//*********************************************************************
//                      songIsKnown
//*********************************************************************

bool Player::songIsKnown(int song) const {
    return(songs[song/8] & 1<<(song%8));
}

//*********************************************************************
//                      learnSong
//*********************************************************************

void Player::learnSong(int song) {
    songs[song/8] |= 1<<(song%8);
}

//*********************************************************************
//                      forgetSong
//*********************************************************************

void Player::forgetSong(int song) {
    songs[song/8] &= ~(1<<(song%8));
}

//*********************************************************************
//                      getDisplayRace
//*********************************************************************

unsigned short Creature::getDisplayRace() const {
    if(isMonster() || !isEffected("illusion"))
        return(race);
    return(getEffect("illusion")->getExtra());
}

//*********************************************************************
//                      isRace
//*********************************************************************
// lets us determine race equality with the presence of subraces

bool isRace(int subRace, int mainRace) {
    return(mainRace == subRace || (mainRace != 0 && gConfig->getRace(subRace)->getParentRace() == mainRace));
}

bool Creature::isRace(int r) const {
    return(::isRace(race, r));
}

//*********************************************************************
//                      hisHer
//*********************************************************************

const char *Creature::hisHer() const {
    Sex sex = getSex();
    if(sex == SEX_FEMALE)
        return("her");
    else if(sex == SEX_MALE)
        return("his");
    return("its");
}

//*********************************************************************
//                      himHer
//*********************************************************************

const char *Creature::himHer() const {
    Sex sex = getSex();
    if(sex == SEX_FEMALE)
        return("her");
    else if(sex == SEX_MALE)
        return("him");
    return("it");
}

//*********************************************************************
//                      heShe
//*********************************************************************

const char *Creature::heShe() const {
    Sex sex = getSex();
    if(sex == SEX_FEMALE)
        return("she");
    else if(sex == SEX_MALE)
        return("he");
    return("it");
}

//*********************************************************************
//                      upHisHer
//*********************************************************************

const char *Creature::upHisHer() const {
    Sex sex = getSex();
    if(sex == SEX_FEMALE)
        return("Her");
    else if(sex == SEX_MALE)
        return("His");
    return("Its");
}

//*********************************************************************
//                      upHeShe
//*********************************************************************

const char *Creature::upHeShe() const {
    Sex sex = getSex();
    if(sex == SEX_FEMALE)
        return("She");
    else if(sex == SEX_MALE)
        return("He");
    return("It");
}

//********************************************************************
//                      getClassString
//********************************************************************

bstring Player::getClassString() const {
    std::ostringstream cStr;
    cStr << get_class_string(static_cast<int>(cClass));
    if(hasSecondClass())
        cStr << "/" << get_class_string(static_cast<int>(cClass2));
    return(cStr.str());
}

//********************************************************************
//                      isBrittle
//********************************************************************

bool Creature::isBrittle() const {
    return(isPlayer() && cClass == CreatureClass::LICH);
}

//********************************************************************
//                      isUndead
//********************************************************************

bool Creature::isUndead() const {
    if(cClass == CreatureClass::LICH)
        return(true);
    if(isEffected("vampirism"))
        return(true);

    if(isMonster()) {
        if(monType::isUndead(type))
            return(true);
        if(flagIsSet(M_UNDEAD))
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      isUnconscious
//*********************************************************************
// unconsciousness provides some special protection in combat for which
// sleeping people do not qualify. use this function when being unconscious
// will help

bool Creature::isUnconscious() const {
    if(isMonster())
        return(false);
    return(flagIsSet(P_UNCONSCIOUS) && !flagIsSet(P_SLEEPING));
}

//*********************************************************************
//                      knockUnconscious
//*********************************************************************

void Creature::knockUnconscious(long duration) {
    if(isMonster())
        return;

    setFlag(P_UNCONSCIOUS);
    clearFlag(P_SLEEPING);
    lasttime[LT_UNCONSCIOUS].ltime = time(0);
    lasttime[LT_UNCONSCIOUS].interval = duration;
}

//*********************************************************************
//                      isBraindead
//*********************************************************************

bool Creature::isBraindead()  const {
    if(isMonster())
        return(false);
    return(flagIsSet(P_BRAINDEAD));
}

//*********************************************************************
//                      fullName
//*********************************************************************

bstring Creature::fullName() const {
    const Player *player = getAsConstPlayer();
    bstring str = getName();

    if(player && !player->getProxyName().empty())
        str += "(" + player->getProxyName() + ")";

    if(player && player->flagIsSet(P_CHOSEN_SURNAME) && player->getSurname() != "") {
        str += " ";
        str += player->getSurname();
    }

    return(str);
}

//*********************************************************************
//                      unmist
//*********************************************************************

void Creature::unmist() {
    if(isMonster())
        return;
    if(!isEffected("mist"))
        return;

    removeEffect("mist");
    clearFlag(P_SNEAK_WHILE_MISTED);
    unhide();

    printColor("^mYou return to your natural form.\n");
    if(getRoomParent())
        broadcast(getSock(), getRoomParent(), "%M reforms.", this);
}

//*********************************************************************
//                      unhide
//*********************************************************************

bool Creature::unhide(bool show) {
    if(isMonster()) {
        if(!flagIsSet(M_HIDDEN))
            return(false);

        clearFlag(M_HIDDEN);
        setFlag(M_WAS_HIDDEN);
    } else {
        if(!flagIsSet(P_HIDDEN))
            return(false);

        clearFlag(P_HIDDEN);

        if(show)
            printColor("^cYou have become unhidden.\n");
    }
    return(true);
}

//*********************************************************************
//                      getBaseRealm
//*********************************************************************

Realm Monster::getBaseRealm() const {
    return(baseRealm);
}

//*********************************************************************
//                      setBaseRealm
//*********************************************************************

void Monster::setBaseRealm(Realm toSet) {
    baseRealm = toSet;
}

//*********************************************************************
//                      hasMp
//*********************************************************************

bool Creature::hasMp()  {
    return(mp.getMax() != 0 && cClass != CreatureClass::BERSERKER && cClass != CreatureClass::LICH);
}

//*********************************************************************
//                      doesntBreathe
//*********************************************************************

bool Creature::doesntBreathe() const {
    return( (monType::noLivingVulnerabilities(type) || isUndead()) &&
            !isEffected("vampirism") // vampires breathe (and can drown)
    );
}

//*********************************************************************
//                      immuneCriticals
//*********************************************************************

bool Creature::immuneCriticals() const {
    return(monType::immuneCriticals(type));
}

//*********************************************************************
//                      getSock
//*********************************************************************

Socket* Creature::getSock() const {
    return(nullptr);
}

//*********************************************************************
//                      getSock
//*********************************************************************

Socket* Player::getSock() const {
    return(mySock);
}

//*********************************************************************
//                      setSock
//*********************************************************************

void Player::setSock(Socket* pSock) {
    mySock = pSock;
}

//*********************************************************************
//                      canSpeak
//*********************************************************************

bool Creature::canSpeak() const {
    if(isStaff())
        return(true);
    if(isEffected("silence"))
        return(false);
    if(getParent()->isEffected("globe-of-silence"))
        return(false);
    return(true);
}

//*********************************************************************
//                      unBlind
//*********************************************************************

void Creature::unBlind() {
    if(getAsPlayer())
        clearFlag(P_DM_BLINDED);
}

//*********************************************************************
//                      unSilence
//*********************************************************************

void Creature::unSilence() {
    if(getAsPlayer())
        clearFlag(P_DM_SILENCED);
}

//*********************************************************************
//                      poisonedByMonster
//*********************************************************************

bool Creature::poisonedByMonster() const {
    return(pFlagIsSet(P_POISONED_BY_MONSTER));
}

//*********************************************************************
//                      poisonedByPlayer
//*********************************************************************

bool Creature::poisonedByPlayer() const {
    return(pFlagIsSet(P_POISONED_BY_PLAYER));
}

//*********************************************************************
//                      getLocation
//*********************************************************************

Location Creature::getLocation() {
    return(currentLocation);
}

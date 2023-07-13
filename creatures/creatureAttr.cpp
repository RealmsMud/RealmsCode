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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <boost/algorithm/string/replace.hpp>  // for replace_all
#include <boost/algorithm/string/trim.hpp>     // for trim
#include <boost/iterator/iterator_traits.hpp>  // for iterator_value<>::type
#include <cstring>                             // for memset, strcpy
#include <ctime>                               // for time, ctime
#include <deque>                               // for _Deque_iterator
#include <list>                                // for list, operator==, list...
#include <map>                                 // for allocator, operator==
#include <ostream>                             // for char_traits, operator<<
#include <string>                              // for string, operator==
#include <string_view>                         // for string_view
#include <utility>                             // for pair, make_pair

#include "anchor.hpp"                          // for Anchor
#include "area.hpp"                            // for MapMarker
#include "calendar.hpp"                        // for cDay, Calendar
#include "carry.hpp"                           // for Carry
#include "catRef.hpp"                          // for CatRef
#include "config.hpp"                          // for Config, gConfig
#include "dice.hpp"                            // for Dice
#include "effects.hpp"                         // for Effects, EffectInfo
#include "flags.hpp"                           // for M_MALE, M_SEXLESS, P_MALE
#include "global.hpp"                          // for CreatureClass, Creatur...
#include "group.hpp"                           // for GROUP_NO_STATUS
#include "hooks.hpp"                           // for Hooks
#include "lasttime.hpp"                        // for lasttime
#include "location.hpp"                        // for Location
#include "monType.hpp"                         // for PLAYER, immuneCriticals
#include "money.hpp"                           // for Money
#include "mud.hpp"                             // for LT_UNCONSCIOUS
#include "mudObjects/container.hpp"            // for Container, ObjectSet
#include "mudObjects/creatures.hpp"            // for Creature, SkillMap
#include "mudObjects/monsters.hpp"             // for Monster, Monster::mob_...
#include "mudObjects/players.hpp"              // for Player, Player::QuestC...
#include "mudObjects/rooms.hpp"                // for BaseRoom
#include "mudObjects/uniqueRooms.hpp"          // for UniqueRoom
#include "proto.hpp"                           // for zero, broadcast, get_c...
#include "quests.hpp"                          // for QuestCompleted, QuestC...
#include "raceData.hpp"                        // for RaceData
#include "range.hpp"                           // for Range
#include "random.hpp"                          // for Random
#include "realm.hpp"                           // for Realm, MAX_REALM, MIN_...
#include "size.hpp"                            // for Size, NO_SIZE, MAX_SIZE
#include "skills.hpp"                          // for Skill
#include "specials.hpp"                        // for SpecialAttack
#include "statistics.hpp"                      // for Statistics
#include "stats.hpp"                           // for Stat
#include "structs.hpp"                         // for SEX_FEMALE, SEX_MALE, Sex
#include "threat.hpp"                          // for ThreatTable
#include "timer.hpp"                           // for Timer
#include "version.hpp"                         // for VERSION
#include "xml.hpp"                             // for copyPropToString
#include "server.hpp"

//*********************************************************************
//                      getClass
//*********************************************************************

CreatureClass Creature::getClass() const { return(cClass); }

int Creature::getClassInt() const { return(static_cast<int>(cClass)); }

int Creature::getSecondClassInt() const {
    const std::shared_ptr<const Player> pThis = getAsConstPlayer();

    if(pThis)
        return(static_cast<int>(pThis->getSecondClass()));
    else
        return (static_cast<int>(CreatureClass::NONE));
}


unsigned short Creature::getLevel() const { return(level); }

short Creature::getAlignment() const { return(alignment); }

int Creature::getArmor() const { return(armor); }

unsigned long Creature::getExperience() const { return(experience); }

std::string Player::getCoinDisplay() const { return(coins.str()); }
std::string Player::getBankDisplay() const { return(bank.str()); }

unsigned short Creature::getClan() const { return(clan); }

mType Creature::getType() const { return(type); }

unsigned short Creature::getRace() const { return(race); }
unsigned short Creature::getDeity() const { return(deity); }

Size Creature::getSize() const { return(size); }

int Creature::getAttackPower() const { return(attackPower); }

std::string Creature::getDescription() const { return(description); }
std::string Creature::getVersion() const { return(version); }

unsigned short Creature::getPoisonDuration() const { return(poison_dur); }
unsigned short Creature::getPoisonDamage() const { return(poison_dmg); }

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

unsigned long Creature::getRealm(Realm r) const { return(realm[r-1]); }

std::string Creature::getPoisonedBy() const { return(poisonedBy); }

bool Creature::inJail() const {
    if(isStaff())
        return(false);
    if(!inUniqueRoom())
        return(false);
    return(getConstUniqueRoomParent()->flagIsSet(R_JAIL));
}

void Creature::addExperience(unsigned long e) {
    setExperience(experience + e);
    if(isPlayer())
        getAsPlayer()->checkLevel();
}

void Creature::subExperience(unsigned long e) {
    setExperience(e > experience ? 0 : experience - e);
    unsigned long lost = (e > experience ? 0 : e);
    if(isPlayer()) {
        getAsPlayer()->checkLevel();
        getAsPlayer()->statistics.experienceLost(lost);
    }
}

//*********************************************************************
//                      setExperience
//*********************************************************************

void Creature::setExperience(unsigned long e) { experience = std::min<unsigned long>(2100000000, e); }

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

void Creature::setClan(unsigned short c) { clan = c; }

void Creature::setLevel(unsigned short l, bool isDm) { level = std::max(1, std::min<int>(l, isDm ? 127 : MAXALVL)); }

void Creature::setAlignment(short a) { alignment = std::max<short>(-1000, std::min<short>(1000, a)); }


void Creature::subAlignment(unsigned short a) { setAlignment(alignment - a); }


void Creature::setArmor(int a) { armor = std::max<int>(std::min(a, MAX_ARMOR), 0); }


void Creature::setAttackPower(int a) { attackPower = std::min<int>(1500, a); }


void Creature::setDeity(unsigned short d) { deity = std::min<unsigned short>(d, DEITY_COUNT-1); }


void Creature::setRace(unsigned short r) { race = std::min<unsigned short>(gConfig->raceCount()-1, r); }


void Creature::setSize(Size s) { size = std::max(NO_SIZE, std::min(MAX_SIZE, s)); }


void Creature::setType(unsigned short t) { type = (mType)std::min<short>(MAX_MOB_TYPES-1, t); }


void Creature::setType(mType t) { type = t; }


void Creature::setDescription(std::string_view desc) {
    description = desc;
    if(isMonster())
        boost::replace_all(description, "*CR*", "\n");
}


void Creature::setVersion(std::string_view v) { version = v.empty() ? VERSION : v; }

void Creature::setVersion(xmlNodePtr rootNode) { xml::copyPropToString(version, rootNode, "Version"); }


void Creature::setPoisonDuration(unsigned short d) { poison_dur = d; }


void Creature::setPoisonDamage(unsigned short d) { poison_dmg = d; }

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


std::string getSexName(Sex sex) {
    if(sex == SEX_FEMALE)
        return("Female");
    if(sex == SEX_MALE)
        return("Male");
    if(sex == SEX_NONE)
        return("Genderless");
    return("Unknown");
}


void Creature::setDeathType(DeathType d) { deathtype = d; }


void Creature::setRealm(unsigned long num, Realm r) { realm[r-1] = std::min<unsigned long>(10000000, num); }
void Creature::addRealm(unsigned long num, Realm r) { setRealm(getRealm(r) + num, r); }
void Creature::subRealm(unsigned long num, Realm r) { setRealm(num > getRealm(r) ? 0 : getRealm(r) - num, r); }

void Creature::setPoisonedBy(std::string_view p) { poisonedBy = p; }

unsigned short Monster::getMobTrade() const { return(mobTrade); }

int Monster::getSkillLevel() const { return(skillLevel); }

int Monster::getMaxLevel() const { return(maxLevel); }

unsigned short Monster::getNumWander() const { return(numwander); }
unsigned short Monster::getLoadAggro() const { return(loadAggro); }
unsigned short Monster::getUpdateAggro() const { return(updateAggro); }
unsigned short Monster::getCastChance() const { return(cast); }
unsigned short Monster::getMagicResistance() const { return(magicResistance); }

std::string Monster::getPrimeFaction() const { return(primeFaction); }
std::string Monster::getTalk() const { return(talk); }

void Monster::setMaxLevel(unsigned short l) { maxLevel = std::max<unsigned short>(0, std::min<unsigned short>(l, MAXALVL)); }
void Monster::setCastChance(unsigned short c) { cast = std::max<unsigned short>(0, std::min<unsigned short>(c, 100)); }
void Monster::setMagicResistance(unsigned short m) { magicResistance = std::max<unsigned short>(0, std::min<unsigned short>(100, m)); }
void Monster::setLoadAggro(unsigned short a) { loadAggro = std::max<unsigned short>(0, std::min<unsigned short>(a, 99)); }
void Monster::setUpdateAggro(unsigned short a) { updateAggro = std::max<unsigned short>(1, std::min<unsigned short>(a, 99)); }
void Monster::setNumWander(unsigned short n) { numwander = std::max<unsigned short>(0, std::min<unsigned short>(6, n)); }
void Monster::setSkillLevel(int l) { skillLevel = std::max(0, std::min(100, l)); }
void Monster::setMobTrade(unsigned short t) { mobTrade = std::max<unsigned short>(0,std::min<unsigned short>(MOBTRADE_COUNT-1, t)); }
void Monster::setPrimeFaction(std::string_view f) { primeFaction = f; }
void Monster::setTalk(std::string_view t) { talk = t; boost::replace_all(talk, "*CR*", "\n"); }

CreatureClass Player::getSecondClass() const { return(cClass2); }
bool Player::hasSecondClass() const { return(cClass2 != CreatureClass::NONE); }

unsigned short Player::getGuild() const { return(guild); }
unsigned short Player::getGuildRank() const { return(guildRank); }
unsigned short Player::getActualLevel() const { return(actual_level); }
unsigned short Player::getNegativeLevels() const { return(negativeLevels); }
unsigned short Player::getWimpy() const { return(wimpy); }
unsigned short Player::getTickDamage() const { return(tickDmg); }
unsigned short Player::getWarnings() const { return(warnings); }
unsigned short Player::getPkin() const { return(pkin); }
unsigned short Player::getPkwon() const { return(pkwon); }

int Player::getWrap() const { return(wrap); }

short Player::getLuck() const { return(luck); }

unsigned short Player::getWeaponTrains() const { return(weaponTrains); }

long Player::getLastLogin() const { return(lastLogin); }
long Player::getLastInterest() const { return(lastInterest); }

std::string Player::getLastPassword() const { return(lastPassword); }
std::string Player::getAfflictedBy() const { return(afflictedBy); }
std::string Player::getLastCommunicate() const { return(lastCommunicate); }
std::string Player::getLastCommand() const { return(lastCommand); }
std::string Player::getSurname() const { return(surname); }
std::string Player::getForum() const { return(forum); }

long Player::getCreated() const { return(created); }

std::string Player::getCreatedStr() const {
    std::string str;
    if(created) {
        str = ctime(&created);
        boost::trim(str);
    } else
        str = oldCreated;
    return(str);
}


std::shared_ptr<Monster>  Player::getAlias() const { return(alias_crt); }

cDay* Player::getBirthday() const { return(birthday); }

std::string Player::getAnchorAlias(int i) const { return(anchor[i] ? anchor[i]->getAlias() : ""); }
std::string Player::getAnchorRoomName(int i) const { return(anchor[i] ? anchor[i]->getRoomName() : ""); }

const Anchor* Player::getAnchor(int i) const { return(anchor[i]); }

bool Player::hasAnchor(int i) const { return anchor[i] != nullptr; }
bool Player::isAnchor(int i, const std::shared_ptr<BaseRoom>& room) const { return(anchor[i]->is(room)); }

unsigned short Player::getThirst() const { return(thirst); }

int Player::numDiscoveredRooms() const { return(roomExp.size()); }

int Player::getUniqueObjId() const { return(uniqueObjId); }

void Player::setTickDamage(unsigned short t) { tickDmg = t; }
void Player::setWarnings(unsigned short w) { warnings = w; }
void Player::addWarnings(unsigned short w) { setWarnings(w + warnings); }
void Player::subWarnings(unsigned short w) { setWarnings(w > warnings ? 0 : warnings - w); }
void Player::setWimpy(unsigned short w) { wimpy = w; }
void Player::setActualLevel(unsigned short l) { actual_level = std::max<unsigned short>(1, std::min<unsigned short>(l, MAXALVL)); }
void Player::setSecondClass(CreatureClass c) { cClass2 = c; }
void Player::setGuild(unsigned short g) { guild = g; }
void Player::setGuildRank(unsigned short g) { guildRank = g; }
void Player::setNegativeLevels(unsigned short l) { negativeLevels = std::max<unsigned short>(0, std::min<unsigned short>(exp_to_lev(experience), l)); }
void Player::setLuck(int l) { luck = l; }
void Player::setWeaponTrains(unsigned short t) { weaponTrains = t; }
void Player::subWeaponTrains(unsigned short t) { setWeaponTrains(t > weaponTrains ? 0 : weaponTrains - t); }

void Player::setLastPassword(std::string_view p) { lastPassword = p; }
void Player::setAfflictedBy(std::string_view a) { afflictedBy = a; }
void Player::setLastLogin(long l) { lastLogin = std::max<long>(0, l); }
void Player::setLastInterest(long l) { lastInterest = std::max<long>(0, l); }
void Player::setLastCommunicate(std::string_view c) { lastCommunicate = c; }
void Player::setLastCommand(std::string_view c) { lastCommand = c; boost::trim(lastCommand); }
void Player::setCreated() { created = time(nullptr); }
void Player::setSurname(const std::string& s) { surname = s.substr(0, 20); }
void Player::setForum(std::string_view f) { forum = f; }
void Player::setAlias(std::shared_ptr<Monster>  m) { alias_crt = m; }

void Player::setBirthday() {
    const Calendar* calendar = gConfig->getCalendar();
    delete birthday;
    birthday = new cDay;
    birthday->setYear(calendar->getCurYear() - gConfig->getRace(getRace())->getStartAge());
    birthday->setDay(calendar->getCurDay());
    birthday->setMonth(calendar->getCurMonth());
}

void Player::delAnchor(int i) {
    if(anchor[i]) {
        delete anchor[i];
        anchor[i] = nullptr;
    }
}

void Player::setAnchor(int i, std::string_view a) {
    delAnchor(i);
    anchor[i] = new Anchor(a, Containable::downcasted_shared_from_this<Player>());
}

void Player::setThirst(unsigned short t) { thirst = t; }

int Player::setWrap(int newWrap) {
    if(newWrap < -1)
        newWrap = -1;
    if(newWrap > 500)
        newWrap = 500;

    wrap = newWrap;
    return(wrap);
}

void Player::setCustomColor(CustomColor i, char c) { customColors[i] = c; }

std::shared_ptr<Creature> Creature::getMaster() {
    return((isMonster() ? getAsMonster()->getMaster() : Containable::downcasted_shared_from_this<Creature>()));
}

std::shared_ptr<const Creature> Creature::getConstMaster() const {
    return((isMonster() ? getAsConstMonster()->getMaster() : Containable::downcasted_shared_from_this<Creature>()));
}

std::shared_ptr<Player> Creature::getPlayerMaster() {
    if(!getMaster())
        return(nullptr);
    return(getMaster()->getAsPlayer());
}

std::shared_ptr<const Player> Creature::getConstPlayerMaster() const {
    if(!getConstMaster())
        return(nullptr);
    return(getConstMaster()->getAsConstPlayer());
}

bool Creature::isPlayer() const {
    return(type == PLAYER);
}

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

    playing = nullptr;
    myTarget.reset();

    for(auto it = targetingThis.begin() ; it != targetingThis.end() ; ) {
        if(auto targeter = it->lock()) {
            targeter->clearTarget(false);
        }
        it++;
    }
    targetingThis.clear();

    zero(key, sizeof(key));

    poisonedBy = description = "";
    version = "0.00";

    fd = -1;
    cClass = CreatureClass::NONE;
    level = race = alignment = experience = temp_experience = clan = 0;
    size = NO_SIZE;
    type = PLAYER;
    deathtype = DT_NONE;

    int i;
    coins.zero();
    flags.reset();
    zero(realm, sizeof(realm));
    spells.reset();
    old_quests.reset();
    languages.reset();
    questnum = 0;

    strength.setName("Strength");
    dexterity.setName("Dexterity");
    constitution.setName("Constitution");
    intelligence.setName("Intelligence");
    piety.setName("Piety");

    hp.setName("Hp");
    mp.setName("Mp");

    auto* pThis = dynamic_cast<Player*>(this);
    if(pThis) {
        pThis->focus.setName("Focus");

        constitution.setInfluences(&hp);
        hp.setInfluencedBy(&constitution);

        intelligence.setInfluences(&mp);
        mp.setInfluencedBy(&intelligence);
    }

    for(i=0; i<MAXWEAR; i++)
        ready[i] = nullptr;

    first_tlk = nullptr;

    currentLocation.mapmarker.reset();
    currentLocation.room.clear();

    group = nullptr;
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
    specials.clear();

    for(i=0; i<21; i++)
        misc[i] = 0;
}

//*********************************************************************
//                      reset
//*********************************************************************

void Monster::monReset() {
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

    myMaster.reset();
    updateAggro = 0;
    cast = 0;
    magicResistance = 0;
    jail.clear();

    primeFaction = "";
    cClassAggro.reset();
    raceAggro.reset();
    deityAggro.reset();

    std::list<TalkResponse*>::iterator tIt;
    for(tIt = responses.begin() ; tIt != responses.end() ; tIt++) {
        delete (*tIt);
    }
    responses.clear();
}

//*********************************************************************
//                      reset
//*********************************************************************

void Player::plyReset() {
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

    songs.reset();
    guild = guildRank = 0;
    cClass2 = CreatureClass::NONE;

    int i;
    actual_level = warnings = 0;
    for(i=0; i<MAX_DIMEN_ANCHORS; i++)
        anchor[i] = nullptr;

    negativeLevels = 0;
    birthday = nullptr;

    luck = 0;
    ansi = 0;
    alias_crt = nullptr;
    scared_of = nullptr;
    timeout = 0;
    thirst = 0;

    resetCustomColors();
    lastPawn = nullptr;

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
    gamblingState = {};
}

//*********************************************************************
//                      Creature
//*********************************************************************

Creature::Creature(): MudObject(), ready(MAXWEAR) {
    crtReset();
}

Creature::Creature(Creature &cr): MudObject(cr), ready(MAXWEAR) {
    crtCopy(cr);
}
Creature::Creature(const Creature &cr): MudObject(cr), ready(MAXWEAR) {
    crtCopy(cr);
}

void Creature::crtCopy(const Creature &cr, bool assign) {
    crtReset();
    if(assign) {
        moCopy(cr);
    }

    int     i;
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

    flags = cr.flags;

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

    languages = cr.languages;
    spells = cr.spells;
    old_quests = cr.old_quests;

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
    std::map<std::string, Skill*>::const_iterator csIt;
    for(csIt = cr.skills.begin() ; csIt != cr.skills.end() ; csIt++) {
        crSkill = (*csIt).second;
        skill = new Skill;
        (*skill) = (*crSkill);
        skills[crSkill->getName()] = skill;
    }

    effects.copy(&cr.effects, this);

    std::list<std::string>::const_iterator mIt;
    for(mIt = cr.minions.begin() ; mIt != cr.minions.end() ; mIt++) {
        minions.push_back(*mIt);
    }

    for(const auto& special : cr.specials) {
        specials.push_back(special);
    }

    for(const auto& pet : cr.pets) {
        pets.push_back(pet);
    }


    attackPower = cr.attackPower;
    previousRoom = cr.previousRoom;
}

//*********************************************************************
//                      doCopy
//*********************************************************************

void Player::plyCopy(const Player& cr, bool assign) {
    plyReset();
    if (assign) {
        crtCopy(cr, assign);
    }

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

    songs = cr.songs;

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

    for(const auto& p : cr.questsInProgress) {
        questsInProgress[p.first] = new QuestCompletion(*(p.second));
    }

    for(const auto& qc : cr.questsCompleted) {
        questsCompleted.insert(std::make_pair(qc.first, new QuestCompleted(*qc.second)));
    }

    for(auto& p : cr.knownAlchemyEffects) {
        knownAlchemyEffects.insert(p);
    }
}

//*********************************************************************
//                      doCopy
//*********************************************************************

void Monster::monCopy(const Monster& cr, bool assign) {
    monReset();
    if (assign) {
        crtCopy(cr, assign);
    }

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

Monster::Monster() : MudObject(), Creature(), threatTable(this) {
    monReset();
}

Monster::Monster(Monster& cr) : MudObject(cr), Creature(cr), threatTable(this)  {
    monCopy(cr);
}

Monster::Monster(const Monster& cr) : MudObject(cr), Creature(cr), threatTable(this) {
    monCopy(cr);
}

//*********************************************************************
//                      Monster operators
//*********************************************************************

Monster& Monster::operator=(const Monster& cr) {
    if(&cr != this)
        monCopy(cr, true);
    return(*this);
}

//*********************************************************************
//                      Player
//*********************************************************************

Player::Player() : MudObject(), Creature() {
    plyReset();
}

Player::Player(Player& cr) : MudObject(cr), Creature(cr) {
    // We want a copy of what we're getting, so clear out anything that was here before
    plyCopy(cr);
}

Player::Player(const Player& cr) : MudObject(cr), Creature(cr)  {
    // We want a copy of what we're getting, so clear out anything that was here before
    plyCopy(cr);
}

//*********************************************************************
//                      Player operators
//*********************************************************************

Player& Player::operator=(const Player& cr) {
    std::clog << "Player=" << cr.getName() << std::endl;
    if(&cr != this)
        plyCopy(cr, true);
    return(*this);
}


//*********************************************************************
//                      Monster
//*********************************************************************

Monster::~Monster() {
    if(gServer->isActive(this))
        gServer->delActive(this);
    for(auto it = responses.begin(); it != responses.end();) {
    	auto response = (*it);
    	it++;
    	delete response;
    }
    responses.clear();
    specials.clear();
}

//*********************************************************************
//                      Player
//*********************************************************************

Player::~Player() {
    int i = 0;

    if(birthday) {
        delete birthday;
        birthday = nullptr;
    }

    for(i=0; i<MAX_DIMEN_ANCHORS; i++) {
        if(anchor[i]) {
            delete anchor[i];
            anchor[i] = nullptr;
        }
    }

    for(const auto& qp : questsInProgress) {
        delete qp.second;
    }
    for(const auto& qc : questsCompleted) {
        delete qc.second;
    }

    questsInProgress.clear();
    questsCompleted.clear();
    setLastPawn(nullptr);
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
    return flags.test(flag);
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
    flags.set(flag);
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
    flags.reset(flag);
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
    flags.flip(flag);
    return(flagIsSet(flag));
}

//*********************************************************************
//                      languageIsKnown
//*********************************************************************

bool Creature::languageIsKnown(int lang) const {
    return(languages.test(lang));
}

//*********************************************************************
//                      learnLanguage
//*********************************************************************

void Creature::learnLanguage(int lang) {
    languages.set(lang);
}

//*********************************************************************
//                      forgetLanguage
//*********************************************************************

void Creature::forgetLanguage(int lang) {
    languages.reset(lang);
}

//*********************************************************************
//                      spellIsKnown
//*********************************************************************

bool Creature::spellIsKnown(int spell) const {
    return(spells.test(spell));
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
    spells.set(spell);
}

//*********************************************************************
//                      forgetSpell
//*********************************************************************

void Creature::forgetSpell(int spell) {
    spells.reset(spell);
}

//*********************************************************************
//                      questIsSet
//*********************************************************************

bool Player::questIsSet(int quest) const {
    return(old_quests.test(quest));
}

//*********************************************************************
//                      setQuest
//*********************************************************************

void Player::setQuest(int quest) {
    old_quests.set(quest);
}

//*********************************************************************
//                      clearQuest
//*********************************************************************

void Player::clearQuest(int quest) {
    old_quests.reset(quest);
}

//*********************************************************************
//                      songIsKnown
//*********************************************************************

bool Player::songIsKnown(int song) const {
    return(songs.test(song));
}

//*********************************************************************
//                      learnSong
//*********************************************************************

void Player::learnSong(int song) {
    songs.set(song);
}

//*********************************************************************
//                      forgetSong
//*********************************************************************

void Player::forgetSong(int song) {
    songs.reset(song);
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

std::string Player::getClassString() const {
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

//********************************************************************
//                      isPureArcaneCaster
//********************************************************************
bool Creature::isPureArcaneCaster() const {
    switch (getClass()) {
    case CreatureClass::MAGE:
    case CreatureClass::LICH:
        return(true);
        break;
    default:
        return(false);
    }

    if (isMonster()) {
        switch (getType()) {
        case DRAGON:
        case DEMON:
        case DEVIL:
        case DEVA:
            return(true);
            break;
        default:
            return(false);
            break;
        }
    }
    return(false);
}
//********************************************************************
//                      isHybridArcaneCaster
//********************************************************************
bool Creature::isHybridArcaneCaster() const {
    if (isPlayer()) {
        if (getAsConstPlayer()->getSecondClass() == CreatureClass::MAGE)
            return(true);
    }
    switch (getClass()) {
    case CreatureClass::BARD:
    case CreatureClass::PUREBLOOD:
        return(true);
        break;
    default:
        return(false);
    }
    return(false);
}
//********************************************************************
//                      isPureDivineCaster
//********************************************************************
bool Creature::isPureDivineCaster() const {
    switch (getClass()) {
    case CreatureClass::CLERIC:
    case CreatureClass::DRUID:
        return(true);
        break;
    default:
        return(false);
    }

    if (isMonster()) {
        switch (getType()) {
        case DEMON:
        case DEVIL:
        case DEVA:
        case FAERIE:
        case ELEMENTAL:
            return(true);
            break;
        default:
            return(false);
            break;
        }
    }
    return(false);
}

//********************************************************************
//                      isHybridDivineCaster
//********************************************************************
bool Creature::isHybridDivineCaster() const {
    if (isPlayer()) {
        if (getAsConstPlayer()->getSecondClass() == CreatureClass::CLERIC)
        return(true);
    }
    switch (getClass()) {
    case CreatureClass::PALADIN:
    case CreatureClass::DEATHKNIGHT:
    case CreatureClass::RANGER:
        return(true);
        break;
    default:
        return(false);
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
    lasttime[LT_UNCONSCIOUS].ltime = time(nullptr);
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

std::string Creature::fullName() const {
    const std::shared_ptr<const Player> player = getAsConstPlayer();
    std::string str = getName();

    if(player && !player->getProxyName().empty())
        str += "(" + player->getProxyName() + ")";

    if(player && player->flagIsSet(P_CHOSEN_SURNAME) && !player->getSurname().empty()) {
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

std::string Monster::getMobTradeName() const {
    int nIndex = std::max<int>( 0, std::min<int>(this->mobTrade, MOBTRADE_COUNT) );
    return(mob_trade_str[nIndex]);

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

bool Creature::hasSock() const {
    auto ply = getAsConstPlayer();
    if(ply)
        return ply->hasSock();
    return false;
}

std::shared_ptr<Socket> Creature::getSock() const {
    return(nullptr);
}

//*********************************************************************
//                      getSock
//*********************************************************************

bool Player::hasSock() const {
    return(!mySock.expired());
}

std::shared_ptr<Socket> Player::getSock() const {
    return(mySock.lock());
}

//*********************************************************************
//                      setSock
//*********************************************************************

void Player::setSock(std::shared_ptr<Socket> pSock) {
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
    if(getParent() && getParent()->isEffected("globe-of-silence"))
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


//*********************************************************************
//                      getStatusStr
//*********************************************************************
// returns a status string that describes the hp condition of the creature

const char* Creature::getStatusStr(int dmg) {
    int health = hp.getCur() - dmg;

    if(health < 1)
        return "'s dead!";

    switch(std::min<int>(health * 10 / (hp.getMax() ? hp.getMax() : 1), 10)) {
        case 10:
            return("'s unharmed.");
        case 9:
            return("'s relatively unscathed.");
        case 8:
            return("'s a little battered.");
        case 7:
            return("'s getting bruised.");
        case 6:
            return("'s noticeably bleeding.");
        case 5:
            return("'s having some trouble.");
        case 4:
            return(" doesn't look too good.");
        case 3:
            return("'s beginning to stagger.");
        case 2:
            return(" has some nasty wounds.");
        case 1:
            return(" isn't going to last much longer.");
        case 0:
            return(" is about to die!");
    }
    return("");
}


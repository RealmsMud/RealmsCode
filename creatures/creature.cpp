/*
 * creature.cpp
 *   Routines that act on creatures
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

#include <fcntl.h>                               // for open, O_RDONLY
#include <unistd.h>                              // for close, read
#include <boost/algorithm/string/case_conv.hpp>  // for to_lower_copy
#include <boost/iterator/iterator_facade.hpp>    // for operator!=
#include <cstdio>                                // for sprintf
#include <cstdlib>                               // for malloc, realloc
#include <cstring>                               // for strcpy
#include <ctime>                                 // for time
#include <list>                                  // for operator==, _List_it...
#include <map>                                   // for operator==, _Rb_tree...
#include <ostream>                               // for operator<<, basic_os...
#include <set>                                   // for set, set<>::iterator
#include <string>                                // for string, operator<<
#include <string_view>                           // for operator<<, string_view
#include <utility>                               // for pair
#include <fstream>
#include <streambuf>

#include "area.hpp"                              // for MapMarker
#include "calendar.hpp"                          // for Calendar
#include "carry.hpp"                             // for Carry
#include "catRef.hpp"                            // for CatRef
#include "catRefInfo.hpp"                        // for CatRefInfo
#include "config.hpp"                            // for Config, gConfig
#include "creatureStreams.hpp"                   // for Streamable
#include "dice.hpp"                              // for Dice
#include "flags.hpp"                             // for M_DM_FOLLOW, M_SNEAKING
#include "global.hpp"                            // for CreatureClass, BRE, DEA
#include "group.hpp"                             // for Group
#include "lasttime.hpp"                          // for lasttime, CRLastTime
#include "location.hpp"                          // for Location
#include "money.hpp"                             // for GOLD, Money
#include "move.hpp"                              // for getRoom, getString
#include "mud.hpp"                               // for LT_SPELL, LT_MON_WANDER
#include "mudObjects/areaRooms.hpp"              // for AreaRoom
#include "mudObjects/container.hpp"              // for ObjectSet, MonsterSet
#include "mudObjects/creatures.hpp"              // for Creature, PetList
#include "mudObjects/exits.hpp"                  // for Exit
#include "mudObjects/monsters.hpp"               // for Monster
#include "mudObjects/objects.hpp"                // for Object
#include "mudObjects/players.hpp"                // for Player
#include "mudObjects/rooms.hpp"                  // for BaseRoom, ExitList
#include "mudObjects/uniqueRooms.hpp"            // for UniqueRoom
#include "paths.hpp"                             // for Desc
#include "proto.hpp"                             // for broadcast, bonus
#include "raceData.hpp"                          // for RaceData
#include "random.hpp"                            // for Random
#include "specials.hpp"                          // for SpecialAttack
#include "server.hpp"                            // for Server, gServer
#include "size.hpp"                              // for getSizeName, NO_SIZE
#include "statistics.hpp"                        // for Statistics
#include "stats.hpp"                             // for Stat
#include "structs.hpp"                           // for saves, ttag
#include "threat.hpp"                            // for ThreatTable, operator<<
#include "xml.hpp"                               // for loadMonster

const short Creature::OFFGUARD_REMOVE=0;
const short Creature::OFFGUARD_NOREMOVE=1;
const short Creature::OFFGUARD_NOPRINT=2;

//*********************************************************************
//                      sameRoom
//*********************************************************************

bool Creature::inSameRoom(const std::shared_ptr<const Creature> &b) const {
    return(getConstRoomParent() == b->getConstRoomParent());
}

//*********************************************************************
//                      getFirstAggro
//*********************************************************************

std::shared_ptr<Monster> getFirstAggro(std::shared_ptr<Monster>  creature, const std::shared_ptr<const Creature> & player) {
    std::shared_ptr<Monster> foundCrt=nullptr;

    if(creature->isPlayer())
        return(creature);

    for(const auto& mons : creature->getConstRoomParent()->monsters) {
        if(mons->getName() != creature->getName())
            continue;

        if(mons->getAsMonster()->isEnemy(player)) {
            foundCrt = mons;
            break;
        }
    }

    if(foundCrt)
        return(foundCrt);
    else
        return(creature);
}

//**********************************************************************
//* AddEnemy
//**********************************************************************

// Add target to the threat list of this monster

bool Monster::addEnemy(const std::shared_ptr<Creature>& target, bool print) {
    if(isEnemy(target))
        return false;

    if(print) {
      if(aggroString[0])
          broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M says, \"%s.\"", this, aggroString);

      target->printColor("^r%M attacks you.\n", this);
      broadcast(target->getSock(), getRoomParent(), "%M attacks %N.", this, target.get());
    }

    if(target->isPlayer()) {
      // take pity on these people
      if(target->isEffected("petrification") || target->isUnconscious())
          return(false);
      // pets should not attack master
      if(isPet() && getMaster() == target)
          return(false);
    }
    if(target->isPet()) {
        addEnemy(target->getMaster());
    }

    adjustThreat(target, 0);
    return(true);
}
bool Monster::isEnemy(const Creature* target) const {
    return(threatTable.isEnemy(target));
}
bool Monster::isEnemy(const std::shared_ptr<const Creature> & target) const {
    return(threatTable.isEnemy(target));
}

bool Monster::hasEnemy() const {
    return(threatTable.hasEnemy());
}

long Monster::adjustContribution(const std::shared_ptr<Creature>& target, long modAmt) {
    return(threatTable.adjustThreat(target, modAmt, 0));
}
long Monster::adjustThreat(const std::shared_ptr<Creature>& target, long modAmt, double threatFactor) {
    target->checkTarget(Containable::downcasted_shared_from_this<Creature>());
    return(threatTable.adjustThreat(target, modAmt, threatFactor));
}
long Monster::clearEnemy(const std::shared_ptr<Creature>& target) {
    return(threatTable.removeThreat(target));
}
//**********************************************************************
//* getTarget
//**********************************************************************
// Returns the first valid creature with the highest threat
// (in the same room if sameRoom == true)
std::shared_ptr<Creature> Monster::getTarget(bool sameRoom) {
    return(threatTable.getTarget(sameRoom));
}


//*********************************************************************
//                      nearEnemy
//*********************************************************************

bool Monster::nearEnemy() const {
    if(nearEnmPly())
        return(true);

    for(const auto& mons : getConstRoomParent()->monsters) {
        if(isEnemy(mons) && !mons->flagIsSet(M_FAST_WANDER))
            return(true);
    }

    return(false);
}

//*********************************************************************
//                      nearEnmPly
//*********************************************************************

bool Monster::nearEnmPly() const {
    for(const auto& pIt: getConstRoomParent()->players) {
        if(auto ply = pIt.lock()) {
            if (isEnemy(ply) && !ply->flagIsSet(P_DM_INVIS))
                return (true);
        }
    }

    return(false);
}

//*********************************************************************
//                      nearEnemy
//*********************************************************************
// Searches for a near enemy, excluding the current player being attacked.

bool Monster::nearEnemy(const std::shared_ptr<Creature> & target) const {
    const std::shared_ptr<const BaseRoom> room = getConstRoomParent();

    for(const auto& pIt: room->players) {
        if(auto ply = pIt.lock()) {
            if (isEnemy(ply) && !ply->flagIsSet(P_DM_INVIS) && target->getName() != ply->getName())
                return (true);
        }

    }

    for(const auto& mons : room->monsters) {
        if(isEnemy(mons) && !mons->flagIsSet(M_FAST_WANDER))
            return(true);

    }

    return(false);
}


//*********************************************************************
//                      tempPerm
//*********************************************************************

void Object::tempPerm() {
    setFlag(O_PERM_INV_ITEM);
    setFlag(O_TEMP_PERM);
    for(const auto& obj : objects) {
        obj->tempPerm();
    }
}

//*********************************************************************
//                      diePermCrt
//*********************************************************************
// This function is called whenever a permanent monster is killed. The
// room the monster was in has its permanent monster list checked to
// if the monster was loaded from that room. If it was, then the last-
// time field for that permanent monster is updated.

void Monster::diePermCrt() {
    std::map<int, CRLastTime>::iterator it;
    CRLastTime* crtm;
    std::shared_ptr<Monster> temp_mob=nullptr;
    std::shared_ptr<UniqueRoom> room;
    long    t = time(nullptr);
    auto pName = getName();

    if(!inUniqueRoom())
        return;
    room = getUniqueRoomParent();

    for(it = room->permMonsters.begin(); it != room->permMonsters.end() ; it++) {
        crtm = &(*it).second;
        if(!crtm->cr.id)
            continue;
        if(crtm->ltime + crtm->interval > t)
            continue;
        if(!loadMonster(crtm->cr, temp_mob))
            continue;
        if(temp_mob->getName() == getName()) {
            crtm->ltime = t;
            temp_mob->monReset();
            break;
        }
        temp_mob->monReset();
    }

    if(flagIsSet(M_DEATH_SCENE) && !flagIsSet(M_FOLLOW_ATTACKER)) {
        std::replace(pName.begin(), pName.end(), ' ', '_');

        fs::path file = Path::Desc / fmt::format("{}_{}", pName, level);
        if(fs::exists(file)) {
            std::ifstream f(file);
            std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

            broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "\n%s", str.c_str());
        }
    }
}

//*********************************************************************
//                      consider
//*********************************************************************
// This function allows the player pointed to by the first argument to
// consider how difficult it would be to kill the creature in the
// second parameter.

std::string Player::consider(const std::shared_ptr<Creature>& creature) const {
    int     diff=0;
    std::string str;

    if(creature->mFlagIsSet(M_UNKILLABLE))
        return("");

    // staff always needle
    if(creature->isStaff() && !isStaff())
        diff = -4;
    else if(isStaff() && !creature->isStaff())
        diff = 4;
    else {
        diff = level - creature->getLevel();
        diff = std::max(-4, std::min(4, diff));
    }

    // upHeShe is used most often
    str = creature->upHeShe();

    switch(diff) {
    case 0:
        str += " is a perfect match for you!\n";
        break;
    case 1:
        str += " is not quite as good as you.\n";
        break;
    case -1:
        str += " is a little better than you.\n";
        break;
    case 2:
        str += " shouldn't be too tough to kill.\n";
        break;
    case -2:
        str += " might be tough to kill.\n";
        break;
    case 3:
        str += " should be easy to kill.\n";
        break;
    case -3:
        str += " should be really hard to kill.\n";
        break;
    case 4:
        str = "You could kill ";
        str += creature->himHer();
        str += " with a needle.\n";
        break;
    case -4:
        str += " could kill you with a needle.\n";
        break;
    }
    return(str);
}

//********************************************************************
//                      hasCharm
//********************************************************************
// This function returns true if the name passed in the first parameter
// is in the charm list of the creature

bool Creature::hasCharm(const std::string &charmed) {
    if(charmed.empty())
        return(false);

    std::shared_ptr<Player> player = getAsPlayer();

    if(!player)
        return(false);

    return (player->charms.find(charmed) != player->charms.end());
}

//********************************************************************
//                      addCharm
//********************************************************************
// This function adds the creature pointed to by the first
// parameter to the charm list of the creature

int Player::addCharm(const std::shared_ptr<Creature>& creature) {
    if(!creature)
        return(0);

    if(hasCharm(creature->getName()))
        return(0);

    if(!creature->isDm()) {
        charms.insert(creature->getName());
        return(1);
    }
    return(0);
}

//********************************************************************
//                      delCharm
//********************************************************************
// This function deletes the creature pointed to by the first
// parameter from the charm list of the creature

int Player::delCharm(const std::shared_ptr<Creature>& creature) {
    if(!creature)
        return(0);

    if(!hasCharm(creature->getName()))
        return(0);

    charms.erase(creature->getName());
    return(1);
}

//*********************************************************************
//                      mobileEnter
//*********************************************************************

bool mobileEnter(const std::shared_ptr<Exit>& exit) {
    return( !exit->flagIsSet(X_SECRET) &&
        !exit->flagIsSet(X_NO_SEE) &&
        !exit->flagIsSet(X_LOCKED) &&
        !exit->flagIsSet(X_PASSIVE_GUARD) &&
        !exit->flagIsSet(X_LOOK_ONLY) &&
        !exit->flagIsSet(X_NO_WANDER) &&
        !exit->isConcealed() &&
        !exit->flagIsSet(X_DESCRIPTION_ONLY) &&
        !exit->isWall("wall-of-fire") &&
        !exit->isWall("wall-of-force") &&
        !exit->isWall("wall-of-thorns") &&
        !exit->isWall("wall-of-lightning") &&
        !exit->isWall("wall-of-sleet")
    );
}

//*********************************************************************
//                      mobile_crt
//*********************************************************************
// This function provides for self-moving monsters. If
// the M_MOBILE_MONSTER flag is set the creature will move around.

int Monster::mobileCrt() {
    std::shared_ptr<BaseRoom> newRoom=nullptr;
    std::shared_ptr<AreaRoom> caRoom = getAreaRoomParent();
    int     i=0, num=0, ret=0;
    bool    mem=false;


    if( !flagIsSet(M_MOBILE_MONSTER) ||
        flagIsSet(M_PERMENANT_MONSTER) ||
        flagIsSet(M_PASSIVE_EXIT_GUARD)
    )
        return(0);

    if(nearEnemy())
        return(0);

    for(const auto& exit : getRoomParent()->exits) {
        // count up exits
        if(mobileEnter(exit))
            i += 1;
    }

    if(!i)
        return(0);

    num = Random::get(1, i);
    i = 0;
    auto mThis = Containable::downcasted_shared_from_this<Monster>();
    for(const auto& exit : getRoomParent()->exits) {
        if(mobileEnter(exit))
            i += 1;

        if(i == num) {

            // get us out of this room
            if(!Move::getRoom(mThis, exit, newRoom))
                return(0);
            if(newRoom->isAreaRoom()) {
                mem = newRoom->getAsAreaRoom()->getStayInMemory();
                newRoom->getAsAreaRoom()->setStayInMemory(true);
            }

            // make sure there are no problems with the new room
            if(!newRoom)
                return(0);
            if(newRoom->countCrt() >= newRoom->getMaxMobs())
                return(0);

            // no wandering between live/construction areas
            if(newRoom->isConstruction() != getRoomParent()->isConstruction())
                return(0);

            if(exit->flagIsSet(X_CLOSED) && !exit->flagIsSet(X_LOCKED)) {
                broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M just opened the %s.", this, exit->getCName());
                exit->clearFlag(X_CLOSED);
            }

            if(flagIsSet(M_WILL_SNEAK) && flagIsSet(M_HIDDEN))
                setFlag(M_SNEAKING);

            if( flagIsSet(M_SNEAKING) && Random::get(1,100) <= (3+dexterity.getCur())*3) {
                broadcast(::isStaff, getSock(), getRoomParent(), "*DM* %M just snuck to the %s.", this,exit->getCName());
            } else {
                std::shared_ptr<Creature> lookingFor = nullptr;
                if(flagIsSet(M_CHASING_SOMEONE) && hasEnemy() && ((lookingFor = getTarget(false)) != nullptr) ) {

                    broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M %s to the %s^x, looking for %s.",
                        this, Move::getString(mThis).c_str(), exit->getCName(), lookingFor->getCName());
                }
                else
                    broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M just %s to the %s^x.",
                        this, Move::getString(mThis).c_str(), exit->getCName());

                clearFlag(M_SNEAKING);
            }


            // see if we can recycle this room
            deleteFromRoom();
            if(caRoom && newRoom == caRoom && newRoom->isAreaRoom()) {
                newRoom = Move::recycle(newRoom->getAsAreaRoom(), exit);
            }
            addToRoom(newRoom);

            // reset stayInMemory
            if(newRoom->isAreaRoom())
                newRoom->getAsAreaRoom()->setStayInMemory(mem);

            lasttime[LT_MON_WANDER].ltime = time(nullptr);
            lasttime[LT_MON_WANDER].interval = Random::get(5,60);

            ret = 1;
            break;
        }
    }

    if(Random::get(1,100) > 80)
        clearFlag(M_MOBILE_MONSTER);

    return(ret);
}

//*********************************************************************
//                      monsterCombat
//*********************************************************************
// This function should make monsters attack each other

void Monster::monsterCombat(const std::shared_ptr<Monster>& target) {
    broadcast((std::shared_ptr<Socket> )nullptr, getRoomParent(), "%M attacks %N.\n", this, target.get());

    addEnemy(target);
}

//*********************************************************************
//                      mobWield
//*********************************************************************

int Monster::mobWield() {
    std::shared_ptr<Object> returnObject=nullptr;
    int     i=0, found=0;

    if(objects.empty())
        return(0);

    if(ready[WIELD - 1])
        return(0);

    for(const auto& obj : objects) {
        for(i=0;i<10;i++) {
            if(carry[i].info == obj->info) {
                found=1;
                break;
            }
        }

        if(!found) {
            continue;
        }

        if(obj->getWearflag() == WIELD) {
            if( (obj->damage.getNumber() + obj->damage.getSides() + obj->damage.getPlus()) <
                    (damage.getNumber() + damage.getSides() + damage.getPlus())/2 ||
                (obj->getShotsCur() < 1) )
            {
                continue;
            }

            returnObject = obj;
            break;
        }
    }
    if(!returnObject)
        return(0);

    equip(returnObject, WIELD);
    return(1);
}

//*********************************************************************
//                      getMobSave
//*********************************************************************

void Monster::getMobSave() {
    int     lvl=0, cls=0, index=0;

    if(cClass == CreatureClass::NONE)
        cls = 4;    // All mobs default save as fighters.
    else
        cls = static_cast<int>(cClass);

    if((saves[POI].chance +
         saves[DEA].chance +
         saves[BRE].chance +
         saves[MEN].chance +
         saves[SPL].chance) == 0)
    {


        saves[POI].chance = 5;
        saves[DEA].chance = 5;
        saves[BRE].chance = 5;
        saves[MEN].chance = 5;
        saves[SPL].chance = 5;

        if(race) {
            saves[POI].chance += gConfig->getRace(race)->getSave(POI);
            saves[DEA].chance += gConfig->getRace(race)->getSave(DEA);
            saves[BRE].chance += gConfig->getRace(race)->getSave(BRE);
            saves[MEN].chance += gConfig->getRace(race)->getSave(MEN);
            saves[SPL].chance += gConfig->getRace(race)->getSave(SPL);
        }

        if(level > 1) {
            for(lvl = 2;  lvl <= level; lvl++) {
                index = (lvl - 2) % 10;
                switch (saving_throw_cycle[cls][index]) {
                case POI:
                    saves[POI].chance += 6;
                    break;
                case DEA:
                    saves[DEA].chance += 6;
                    break;
                case BRE:
                    saves[BRE].chance += 6;
                    break;
                case MEN:
                    saves[MEN].chance += 6;
                    break;
                case SPL:
                    saves[SPL].chance += 6;
                    break;
                }

            }
        }

        saves[LCK].chance = ((saves[POI].chance +
            saves[DEA].chance +
            saves[BRE].chance +
            saves[MEN].chance +
            saves[SPL].chance)/5);
    }
}

std::string Creature::getClassString() const {
    return get_class_string(static_cast<int>(cClass));
}

//*********************************************************************
//                      castDelay
//*********************************************************************

void Creature::castDelay(long delay) {
    if(delay < 1)
        return;

    lasttime[LT_SPELL].ltime = time(nullptr);
    lasttime[LT_SPELL].interval = delay;
}

//*********************************************************************
//                      attackDelay
//*********************************************************************

void Creature::attackDelay(long delay) {
    if(delay < 1)
        return;

    getAsPlayer()->updateAttackTimer(true, (int)delay*10);
}

//*********************************************************************
//                      enm_in_group
//*********************************************************************

std::shared_ptr<Creature>enm_in_group(std::shared_ptr<Creature>target) {
    std::shared_ptr<Creature>enemy=nullptr;
    int     chosen=0;


    if(!target || Random::get(1,100) <= 50)
        return(target);

    Group* group = target->getGroup();

    if(!group)
        return(target);

    chosen = Random::get(1, group->getSize());

    enemy = group->getMember(chosen);
    if(!enemy || !enemy->inSameRoom(target))
        enemy = target;

    return(enemy);

}

//*********************************************************************
//                      clearEnemyList
//*********************************************************************

void Monster::clearEnemyList() {
    threatTable.clear();
}

//*********************************************************************
//                      clearMobInventory
//*********************************************************************

void Monster::clearMobInventory() {
    std::shared_ptr<Object>  object=nullptr;
    ObjectSet::iterator it;
    for(it = objects.begin() ; it != objects.end() ; ) {
        object = (*it++);
        this->delObj(object, false, false, true, false);
        object.reset();
    }
    checkDarkness();
}

//*********************************************************************
//                      cleanMobForSaving
//*********************************************************************
// This function will clean up the mob so it is ready for saving to the database
// ie: it will clear the inventory, any flags that shouldn't be saved to the
// database, clear any following etc.

int Monster::cleanMobForSaving() {

    // No saving pets!
    if(isPet())
        return(-1);

    // Clear the inventory
    clearMobInventory();

    // Clear any flags that shouldn't be set
    clearFlag(M_WILL_BE_LOGGED);

//  // If the creature is possessed, clean that up
    if(flagIsSet(M_DM_FOLLOW)) {
        clearFlag(M_DM_FOLLOW);
        std::shared_ptr<Player> master;
        if(getMaster() != nullptr && (master = getMaster()->getAsPlayer()) != nullptr) {
            master->clearFlag(P_ALIASING);
            master->getAsPlayer()->setAlias(nullptr);
            master->print("%1M's soul was saved.\n", this);
            removeFromGroup(false);
        }
    }

    // Success
    return(1);
}

//*********************************************************************
//                      displayFlags
//*********************************************************************

int Creature::displayFlags() const {
    int retFlags = 0;
    if(isEffected("detect-invisible"))
        retFlags |= INV;
    if(isEffected("detect-magic"))
        retFlags |= MAG;
    if(isEffected("true-sight"))
        retFlags |= MIST;
    if(isPlayer()) {
        if(cClass == CreatureClass::BUILDER)
            retFlags |= ISBD;
        if(cClass == CreatureClass::CARETAKER)
            retFlags |= ISCT;
        if(isDm())
            retFlags |= ISDM;
        if(flagIsSet(P_NO_NUMBERS))
            retFlags |= NONUM;
    }
    return(retFlags);
}

//*********************************************************************
//                      displayCreature
//*********************************************************************

int Player::displayCreature(const std::shared_ptr<Creature>& target)  {
    int     percent=0, rank=0, chance=0;
    int flags = displayFlags();
    std::shared_ptr<Player> pTarget = target->getAsPlayer();
    std::shared_ptr<Monster> mTarget = target->getAsMonster();
    std::ostringstream oStr;
    std::string str;
    bool space=false;

    auto pThis = Containable::downcasted_shared_from_this<Player>();
    if(mTarget) {
        oStr << "You see " << mTarget->getCrtStr(pThis, flags, 1) << ".\n";
        if(!mTarget->getDescription().empty())
            oStr << mTarget->getDescription() << "\n";
        else
            oStr << "There is nothing special about " << mTarget->getCrtStr(pThis, flags, 0) << ".\n";

        if(mTarget->getMobTrade()) {
            rank = mTarget->getSkillLevel()/10;
            oStr << "^y" << mTarget->getCrtStr(pThis, flags | CAP, 0) << " is a " << mTarget->getMobTradeName()
                 << ". " << mTarget->upHisHer() << " skill level: " << get_skill_string(rank) << ".^x\n";
        }
    } else if(pTarget) {

        oStr << "You see " << pTarget->fullName() << " the "
             << gConfig->getRace(pTarget->getDisplayRace())->getAdjective();
        // will they see through the illusion?
        if(willIgnoreIllusion() && pTarget->getDisplayRace() != pTarget->getRace())
            oStr << " (" << gConfig->getRace(pTarget->getRace())->getAdjective() << ")";
        oStr << " "
             << pTarget->getTitle() << ".\n";

        if(gConfig->getCalendar()->isBirthday(pTarget)) {
            oStr << "^yToday is " << pTarget->getCName() << "'s birthday! " << pTarget->upHeShe()
                 << " is " << pTarget->getAge() << " years old.^x\n";
        }

        if(!pTarget->description.empty())
            oStr << pTarget->description << "\n";
    }

    if(target->isEffected("vampirism")) {
        chance = intelligence.getCur()/10 + piety.getCur()/10;
        // vampires and werewolves can sense each other
        if(isEffected("vampirism") || isEffected("lycanthropy"))
            chance = 101;
        if(chance > Random::get(0,100)) {
            switch(Random::get(1,4)) {
            case 1:
                oStr << target->upHeShe() << " looks awfully pale.\n";
                break;
            case 2:
                oStr << target->upHeShe() << " has an unearthly presence.\n";
                break;
            case 3:
                oStr << target->upHeShe() << " has hypnotic eyes.\n";
                break;
            default:
                oStr << target->upHeShe() << " looks rather pale.\n";
            }
        }
    }

    if(target->isEffected("lycanthropy")) {
        chance = intelligence.getCur()/10 + piety.getCur()/10;
        // vampires and werewolves can sense each other
        if(isEffected("vampirism") || isEffected("lycanthropy"))
            chance = 101;
        if(chance > Random::get(0,100)) {
            switch(Random::get(1,3)) {
            case 1:
                oStr << target->upHeShe() << " looks awfully shaggy.\n";
                break;
            case 2:
                oStr << target->upHeShe() << " has a feral presence.\n";
                break;
            default:
                oStr << target->upHeShe() << " looks rather shaggy.\n";
            }
        }
    }

    if(target->isEffected("slow"))
        oStr << target->upHeShe() << " is moving very slowly.\n";
    else if(target->isEffected("haste"))
        oStr << target->upHeShe() << " is moving unnaturally quick.\n";


    if((cClass == CreatureClass::CLERIC && deity == JAKAR && level >=7) || isCt())
        oStr << "^y" << target->getCrtStr(pThis, flags | CAP, 0 ) << " is carrying "
             << target->coins[GOLD] << " gold coin"
             << (target->coins[GOLD] != 1 ? "s" : "") << ".^x\n";

    if(isEffected("know-aura") || cClass==CreatureClass::PALADIN) {
        space = true;
        oStr << target->getCrtStr(pThis, flags | CAP, 0) << " ";
        oStr << "has a " << target->alignColor() << target->alignString() << "^x aura.";

    }

    if(mTarget && mTarget->getRace()) {
        if(space)
            oStr << " ";
        space = true;
        oStr << mTarget->upHeShe() << " is ^W"
             << boost::algorithm::to_lower_copy(gConfig->getRace(mTarget->getRace())->getAdjective()) << "^x.";
    }
    if(target->getSize() != NO_SIZE) {
        if(space)
            oStr << " ";
        space = true;
        oStr << target->upHeShe() << " is ^W" << getSizeName(target->getSize()) << "^x.";
    }

    if(space)
        oStr << "\n";

    if(target->hp.getCur() > 0 && target->hp.getMax())
        percent = (100 * (target->hp.getCur())) / (target->hp.getMax());
    else
        percent = -1;

    if(!(mTarget && mTarget->flagIsSet(M_UNKILLABLE))) {
        oStr << target->upHeShe();
        if(percent >= 100 || !target->hp.getMax())
            oStr << " is in excellent condition.\n";
        else if(percent >= 90)
            oStr << " has a few scratches.\n";
        else if(percent >= 75)
            oStr << " has some small wounds and bruises.\n";
        else if(percent >= 60)
            oStr << " is wincing in pain.\n";
        else if(percent >= 35)
            oStr << " has quite a few wounds.\n";
        else if(percent >= 20)
            oStr << " has some big nasty wounds and scratches.\n";
        else if(percent >= 10)
            oStr << " is bleeding awfully from big wounds.\n";
        else if(percent >= 5)
            oStr << " is barely clinging to life.\n";
        else if(percent >= 0)
            oStr << " is nearly dead.\n";
    }

    if(pTarget) {
        if(pTarget->isEffected("mist")) {
            oStr << pTarget->upHeShe() << "%s is currently in mist form.\n";
            printColor("%s", oStr.str().c_str());
            return(0);
        }

        if(pTarget->flagIsSet(P_UNCONSCIOUS))
            oStr << pTarget->getName() << " is "
                 << (pTarget->flagIsSet(P_SLEEPING) ? "sleeping" : "unconscious") << ".\n";

        if(pTarget->isEffected("petrification"))
            oStr << pTarget->getName() << " is petrified.\n";

        if(pTarget->isBraindead())
            oStr << pTarget->getName() << "%M is currently brain dead.\n";
    } else {
        if(mTarget->isEnemy(pThis))
            oStr << mTarget->upHeShe() << " looks very angry at you.\n";
        else if(!mTarget->getPrimeFaction().empty())
            oStr << mTarget->upHeShe() << " " << getFactionMessage(mTarget->getPrimeFaction()) << ".\n";

        std::shared_ptr<Creature> firstEnm = nullptr;
        if((firstEnm = mTarget->getTarget(false)) != nullptr) {
            if(firstEnm.get() == this) {
                if(  !mTarget->flagIsSet(M_HIDDEN) && !(mTarget->isInvisible() && isEffected("detect-invisible")))
                    oStr << mTarget->upHeShe() << " is attacking you.\n";
            } else
                oStr << mTarget->upHeShe() << " is attacking " << firstEnm->getName() << ".\n";

            /// print all the enemies if a CT or DM is looking
            if(isCt())
                oStr << mTarget->threatTable;
        }
        oStr << consider(mTarget);

        // pet code
        if(mTarget->isPet() && mTarget->getMaster() == pThis) {
            str = mTarget->listObjects(pThis, true);
            oStr << mTarget->upHeShe() << " ";
            if(str.empty())
                oStr << "isn't holding anything.\n";
            else
                oStr << "is carrying: " << str << ".\n";
        }
    }
    printColor("%s", oStr.str().c_str());
    target->printEquipList(pThis);
    return(0);
}

//*********************************************************************
//                      checkSpellWearoff
//*********************************************************************

void Monster::checkSpellWearoff() {
    long t = time(nullptr);

    /*
    if(flagIsSet(M_WILL_MOVE_FOR_CASH)) {
        if(t > LT(this, LT_MOB_PASV_GUARD)) {
            // Been 30 secs, time to stand guard again
            setFlag(M_PASSIVE_EXIT_GUARD);
        }
    }
     */

    if(t > LT(this, LT_CHARMED) && flagIsSet(M_CHARMED)) {
        printColor("^y%M's demeanor returns to normal.\n", this);
        clearFlag(M_CHARMED);
    }
}


bool Creature::isSitting() const {
    return(pFlagIsSet(P_SITTING));
}

//*********************************************************************
//                      canFlee
//*********************************************************************

bool Creature::canFlee(bool displayFail, bool checkTimer) {
    bool    crtInRoom=false;
    long    t=0;
    int     i=0;

    if(isMonster()) {

        if(!flagIsSet(M_WILL_FLEE) || flagIsSet(M_DM_FOLLOW))
            return(false);

        if(hp.getCur() > hp.getMax()/5)
            return(false);

        if(flagIsSet(M_PERMENANT_MONSTER))
            return(false);

    } else {
        //std::shared_ptr<Player> pThis = getPlayer();
        if(!ableToDoCommand())
            return(false);

        if(flagIsSet(P_SITTING)) {
            if(displayFail)
                print("You have to stand up first.\n");
            return(false);
        }

        if(isMagicallyHeld(displayFail)) {
            return(false);
        }

        if(checkTimer && !isEffected("fear") && !isStaff()) {
            t = time(nullptr);
            i = std::max(getLTAttack(), std::max(lasttime[LT_SPELL].ltime,lasttime[LT_READ_SCROLL].ltime)) + 3L;
            if(t < i) {
                if(displayFail)
                    pleaseWait(i-t);
                return(false);
            }
        }

        // confusion and drunkenness overrides following checks
        if(isEffected("confusion") || isEffected("drunkenness"))
            return(true);


        // players can only flee if someone/something else is in the room
        for(const auto& mons : getConstRoomParent()->monsters) {
            if(!mons->isPet()) {
                crtInRoom = true;
                break;
            }
        }

        if(!crtInRoom) {
            for(const auto& pIt: getConstRoomParent()->players) {
                auto ply = pIt.lock();
                if(!ply || ply.get() == this)
                    continue;
                if(!ply->isStaff()) {
                    crtInRoom = true;
                    break;
                }
            }
        }

        if( !crtInRoom && !checkStaff("There's nothing to flee from!\n") ) {
            getAsPlayer()->setFleeing(false);
            return(false);
        }
    }

    return(true);
}


//*********************************************************************
//                      canFleeToExit
//*********************************************************************

bool Creature::canFleeToExit(const std::shared_ptr<Exit>& exit, bool skipScary, bool blinking) {
    std::shared_ptr<Player> pThis = getAsPlayer();

    // flags both players and mobs have to obey
    if(!canEnter(exit, false, blinking) ||

        exit->flagIsSet(X_SECRET) ||
        exit->isConcealed(Containable::downcasted_shared_from_this<Creature>()) ||
        exit->flagIsSet(X_DESCRIPTION_ONLY) ||

        exit->flagIsSet(X_NO_FLEE) ||

        exit->flagIsSet(X_TO_STORAGE_ROOM) ||
        exit->flagIsSet(X_TO_BOUND_ROOM) ||
        exit->flagIsSet(X_TOLL_TO_PASS) ||
        exit->flagIsSet(X_WATCHER_UNLOCK)
    )
        return(false);


    if(pThis) {

        if(
            ((  getRoomParent()->flagIsSet(R_NO_FLEE) ||
                getRoomParent()->flagIsSet(R_LIMBO)
            ) && inCombat()) ||
            (exit->flagIsSet(X_DIFFICULT_CLIMB) && !isEffected("levitate")) ||
            (
                (
                    exit->flagIsSet(X_NEEDS_CLIMBING_GEAR) ||
                    exit->flagIsSet(X_CLIMBING_GEAR_TO_REPEL) ||
                    exit->flagIsSet(X_DIFFICULT_CLIMB)
                ) &&
                !isEffected("levitate")
            )
        ) return(false);

        int chance=0, *scary;

        // should we skip checking for scary exits?
        skipScary = skipScary || fd == -1 || exit->target.mapmarker.getArea() || isEffected("fear");

        // are they scared of going in that direction
        if(!skipScary && pThis->scared_of) {
            scary = pThis->scared_of;
            while(*scary) {
                if(exit->target.room.id && *scary == exit->target.room.id) {
                    // oldPrint(fd, "Scared of going %s^x!\n", exit->getCName());

                    // there is a chance we will flee to this exit anyway
                    chance = 65 + bonus(dexterity.getCur()) * 5;

                    if(getRoomParent()->flagIsSet(R_DIFFICULT_TO_MOVE) || getRoomParent()->flagIsSet(R_DIFFICULT_FLEE))
                        chance /=2;

                    if(Random::get(1,100) < chance)
                        return(false);

                    // success; means that the player is now scared of this room
                    if(inUniqueRoom() && fd > -1) {
                        scary = pThis->scared_of;
                        {
                            int roomNum = getUniqueRoomParent()->info.id;
                            if(scary) {
                                int lSize = 0;
                                while(*scary) {
                                    lSize++;
                                    if(*scary == roomNum)
                                        break;
                                    scary++;
                                }
                                if(!*scary) {
                                    // LEAK: Next line reported to be leaky: 10 count
                                    pThis->scared_of = (int *) realloc(pThis->scared_of, (lSize + 2) * sizeof(int));
                                    (pThis->scared_of)[lSize] = roomNum;
                                    (pThis->scared_of)[lSize + 1] = 0;
                                }
                            } else {
                                // LEAK: Next line reported to be leaky: 10 count
                                pThis->scared_of = (int *) malloc(sizeof(int) * 2);
                                (pThis->scared_of)[0] = roomNum;
                                (pThis->scared_of)[1] = 0;
                            }
                        }
                    }
                    return(true);
                }
                scary++;
            }
        }

    }

    return(true);
}


//*********************************************************************
//                      getFleeableExit
//*********************************************************************

std::shared_ptr<Exit> Creature::getFleeableExit() {
    int     i=0, exit=0;
    bool    skipScary=false;

    // count exits so we can randomly pick one

    for(const auto& ext : getRoomParent()->exits) {
        if(canFleeToExit(ext))
            i++;
    }

    if(i) {
        // pick a random exit
        exit = Random::get(1, i);
    } else if(isPlayer()) {
        // force players to skip the scary list
        skipScary = true;
        i=0;

        for(const auto& ext : getRoomParent()->exits) {
            if(canFleeToExit(ext, true))
                i++;
        }
        if(i)
            exit = Random::get(1, i);
    }

    if(!exit)
        return(nullptr);

    i=0;
    for(const auto& ext : getRoomParent()->exits) {
        if(canFleeToExit(ext, skipScary))
            i++;
        if(i == exit)
            return(ext);
    }

    return(nullptr);
}


//*********************************************************************
//                      getFleeableRoom
//*********************************************************************

std::shared_ptr<BaseRoom> Creature::getFleeableRoom(const std::shared_ptr<Exit>& exit) {
    std::shared_ptr<BaseRoom> newRoom=nullptr;
    if(!exit)
        return(nullptr);
    // exit flags have already been taken care of above, so
    // feign teleporting so we don't recycle the room
    Move::getRoom(Containable::downcasted_shared_from_this<Creature>(), exit, newRoom, false, nullptr, false);
    return(newRoom);
}


//*********************************************************************
//                      flee
//*********************************************************************
// This function allows a creature to flee from an enemy. If it's a monster,
// fear induced or because of wimpy, it will try to flee immediately, otherwise it will set
// the player as fleeing and try to flee during the combat update

int Creature::flee(bool magicTerror, bool wimpyFlee) {
    std::shared_ptr<Monster>  mThis = getAsMonster();
    std::shared_ptr<Player> pThis = getAsPlayer();

    if(!magicTerror && !canFlee(true, false))
        return(0);

    if(mThis || magicTerror || wimpyFlee) {
        if(wimpyFlee && pThis)
            pThis->setFleeing(true);
        doFlee(magicTerror);
    } else {
        *this << "You frantically look around for someplace to flee to!\n";
        pThis->setFleeing(true);
    }
    return(1);
}

bool Creature::doFlee(bool magicTerror) {
    std::shared_ptr<Monster>  mThis = getAsMonster();
    std::shared_ptr<Player> pThis = getAsPlayer();
    std::shared_ptr<BaseRoom> oldRoom = getRoomParent(), newRoom=nullptr;
    std::shared_ptr<UniqueRoom> uRoom=nullptr;

    std::shared_ptr<Exit>   exit=nullptr;
    int n=0;

    if(isEffected("fear"))
        magicTerror = true;

    exit = getFleeableExit();
    if(!exit) {
        printColor("You couldn't find any place to run to!\n");
        if(pThis)
            pThis->setFleeing(false);
        return(false);
    }

    newRoom = getFleeableRoom(exit);
    if(!newRoom) {
        printColor("You failed to escape!\n");
        return(false);
    }


    switch(Random::get(1,10)) {
    case 1:
        print("You run like a chicken.\n");
        break;
    case 2:
        print("You flee in terror.\n");
        break;
    case 3:
        print("You run screaming in horror.\n");
        break;
    case 4:
        print("You flee aimlessly in any direction you can.\n");
        break;
    case 5:
        print("You run screaming for your mommy.\n");
        break;
    case 6:
        print("You run as your life flashes before your eyes.\n");
        break;
    case 7:
        print("Your heart throbs as you attempt to escape death.\n");
        break;
    case 8:
        print("Colors and sounds mingle as you frantically flee.\n");
        break;
    case 9:
        print("Fear of death grips you as you flee in panic.\n");
        break;
    case 10:
        print("You run like a coward.\n");
        break;
    default:
        print("You run like a chicken.\n");
        break;
    }

    

    broadcast(getSock(), oldRoom, "%M flees to the %s^x.", this, exit->getCName());
    if(mThis) {
        mThis->diePermCrt();
        if(exit->doEffectDamage(Containable::downcasted_shared_from_this<Creature>()))
            return(true);
        mThis->deleteFromRoom();
        mThis->addToRoom(newRoom);
    } else if(pThis) {
       
        pThis->checkDarkness();
        unhide();


        if(magicTerror)
            printColor("^rYou flee from unnatural fear!\n");

        //Berserk goes away when fleeing
        if (pThis->isEffected("berserk")) {
            print("You've lost your lust for battle!\n");
            pThis->removeEffect("berserk");
        }

        Move::track(getUniqueRoomParent(), currentLocation.mapmarker, exit, pThis, false);

        if(pThis->flagIsSet(P_ALIASING)) {
            pThis->getAlias()->deleteFromRoom();
            pThis->getAlias()->addToRoom(newRoom);
        }

        Move::update(pThis);
        pThis->statistics.flee();

        if(pThis->flagIsSet(P_CLEAR_TARGET_ON_FLEE))
            pThis->clearTarget();

        if(cClass == CreatureClass::PALADIN && deity != GRADIUS && !magicTerror && level >= 10) {
            n = level * 8;
            n = std::min<long>(experience, n);
            print("You lose %d experience for your cowardly retreat.\n", n);
            experience -= n;
        }

        if(exit->doEffectDamage(Containable::downcasted_shared_from_this<Creature>()))
            return(true);
        pThis->deleteFromRoom();
        pThis->addToRoom(newRoom);

        for(const auto& pet : pThis->pets) {
            if(pet && inSameRoom(pThis))
                broadcast(getSock(), oldRoom, "%M flees to the %s^x with its master.", pet.get(), exit->getCName());
        }
    }
    exit->checkReLock(Containable::downcasted_shared_from_this<Creature>(), false);

    broadcast(getSock(), newRoom, "%M just fled rapidly into the room.", this);

    if(pThis) {

        pThis->doPetFollow();
        uRoom = newRoom->getAsUniqueRoom();
        if(uRoom)
            pThis->checkTraps(uRoom);


        if(Move::usePortal(pThis, oldRoom, exit))
            Move::deletePortal(oldRoom, exit);
    }

    return(true);
}


Creature::~Creature() {
    for(auto it = targetingThis.begin() ; it != targetingThis.end() ; ) {
        if(auto targeter = it->lock()) {
            targeter->clearTarget(false);
        }
        it++;
    }
    targetingThis.clear();

    clearTarget();

    factions.clear();

    Skill* skill;
    std::map<std::string, Skill*>::iterator csIt;
    for(csIt = skills.begin() ; csIt != skills.end() ; csIt++) {
        skill = (*csIt).second;
        delete skill;
    }
    skills.clear();

    effects.removeAll();
    minions.clear();
    specials.clear();

    ttag    *tp=nullptr, *tempt=nullptr;
    int i;
    for(i=0; i<MAXWEAR; i++) {
        if(ready[i]) ready[i] = nullptr;
    }

    objects.clear();

    if(getGroup(false)) {
        getGroup(false)->remove(Containable::downcasted_shared_from_this<Creature>());
    }
    for(const auto& mons : pets) {
        if(!mons->isPet()) {
            mons->setMaster(nullptr);
        }
    }
    pets.clear();

    tp = first_tlk;
    first_tlk = nullptr;
    while(tp) {
        tempt = tp->next_tag;
        delete[] tp->key;
        delete[] tp->response;
        delete[] tp->action;
        delete[] tp->target;
        delete tp;
        tp = tempt;
    }


    gServer->removeDelayedActions(this);

}


std::string Monster::getFlagList(std::string_view sep) const {
    std::ostringstream ostr;
    bool found = false;
    for(int i=0; i<MAX_MONSTER_FLAGS; i++) {
        if(flagIsSet(i)) {
            if(found)
                ostr << sep;

            ostr << gConfig->getMFlag(i) << "(" << i+1 << ")";
            found = true;
        }
    }

    if(!found)
        return("None");
    else
        return ostr.str();
}

std::string Player::getFlagList(std::string_view sep) const {
    std::ostringstream ostr;
    bool found = false;
    for(int i=0; i<MAX_PLAYER_FLAGS; i++) {
        if(flagIsSet(i)) {
            if(found)
                ostr << sep;

            ostr << gConfig->getPFlag(i) << "(" << i+1 << ")";
            found = true;
        }
    }

    if(!found)
        return("None");
    else
        return ostr.str();
}



//*********************************************************************
//                      recallWhere
//*********************************************************************
// Because of ethereal plane, we don't always know where we're going to
// recall to. We need a function to figure out where we are going.

std::shared_ptr<BaseRoom> Creature::recallWhere() {
    // A builder should never get this far, but let's not chance it.
    // Only continue if they can't load the perm_low_room.
    if(cClass == CreatureClass::BUILDER) {
        std::shared_ptr<UniqueRoom> uRoom=nullptr;
        CatRef cr;
        cr.setArea("test");
        cr.id = 1;
        if(loadRoom(cr, uRoom))
            return(uRoom);
    }

    if( getRoomParent()->flagIsSet(R_ETHEREAL_PLANE) &&
        (Random::get(1,100) <= 50)
            ) {
        return(teleportWhere());
    }

    std::shared_ptr<BaseRoom> room = getRecallRoom().loadRoom(getAsPlayer());
    // uh oh!
    if(!room)
        return(abortFindRoom(Containable::downcasted_shared_from_this<Creature>(), "recallWhere"));
    return(room);

}


//*********************************************************************
//                      teleportWhere
//*********************************************************************
// Loops through rooms and finds us a place we can teleport to.
// This function will always return a room or it will crash trying to.

std::shared_ptr<BaseRoom> Creature::teleportWhere() {
    std::shared_ptr<BaseRoom> newRoom=nullptr;
    const CatRefInfo* cri = gConfig->getCatRefInfo(getRoomParent());
    int     i=0, zone = cri ? cri->getTeleportZone() : 0;
    std::shared_ptr<Area> area=nullptr;
    Location l;
    bool    found = false;


    auto cThis = Containable::downcasted_shared_from_this<Creature>();
    // A builder should never get this far, but let's not chance it.
    // Only continue if they can't load the perm_low_room.
    if(cClass == CreatureClass::BUILDER) {
        CatRef cr;
        cr.setArea("test");
        cr.id = 1;
        std::shared_ptr<UniqueRoom> uRoom =nullptr;
        if(loadRoom(cr, uRoom))
            return(uRoom);
    }

    do {
        if(i>250)
            return(abortFindRoom(cThis, "teleportWhere"));
        cri = gConfig->getRandomCatRefInfo(zone);

        // if this fails, we have nowhere to teleport to
        if(!cri)
            return(getRoomParent());

        // special area used to signify overland map
        if(cri->getArea() == "area") {
            area = gServer->getArea(cri->getId());
            l.mapmarker.set(area->id, Random::get<short>(0, area->width), Random::get<short>(0, area->height), Random::get<short>(0, area->depth));
            if(area->canPass(nullptr, l.mapmarker, true)) {
                //area->adjustCoords(&mapmarker.x, &mapmarker.y, &mapmarker.z);

                // don't bother sending a creature because we've already done
                // canPass check here
                //aRoom = area->loadRoom(0, &mapmarker, false);
                if(Move::getRoom(cThis, nullptr, newRoom, false, &l.mapmarker)) {
                    if(newRoom->isUniqueRoom()) {
                        // recheck, just to be safe
                        found = newRoom->getAsUniqueRoom()->canPortHere(cThis);
                        if(!found)
                            newRoom = nullptr;
                    } else {
                        found = true;
                    }
                }
            }
        } else {
            l.room.setArea(cri->getArea());
            // if misc, first 1000 rooms are off-limits
            l.room.id = Random::get<short>(l.room.isArea("misc") ? 1000 : 1, cri->getTeleportWeight());
            std::shared_ptr<UniqueRoom> uRoom = nullptr;

            if(loadRoom(l.room, uRoom))
                found = uRoom->canPortHere(cThis);
            if(found)
                newRoom = uRoom;
        }

        i++;
    } while(!found);

    if(!newRoom)
        return(abortFindRoom(cThis, "teleportWhere"));
    return(newRoom);
}


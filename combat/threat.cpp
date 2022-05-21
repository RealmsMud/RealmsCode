/*
 * threat.cpp
 *   Functions that deal with threat and targetting
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

#include <strings.h>                 // for strcasecmp
#include <algorithm>                 // for find
#include <cassert>                   // for assert
#include <ctime>                     // for time
#include <functional>                // for less
#include <iostream>                  // for operator<<, ostream, basic_ostream
#include <iterator>                  // for operator!=, reverse_iterator
#include <list>                      // for list, operator==, _List_iterator
#include <map>                       // for _Rb_tree_const_iterator, operator==
#include <set>                       // for multiset<>::iterator, multiset<>...
#include <string>                    // for operator<<, string, char_traits
#include <string_view>               // for string_view
#include <utility>                   // for pair

#include "cmd.hpp"                   // for cmd
#include "commands.hpp"              // for cmdAssist, cmdTarget
#include "flags.hpp"                 // for P_COMPACT, P_NO_AUTO_TARGET
#include "mudObjects/container.hpp"  // for Container, MonsterSet
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/mudObject.hpp"  // for MudObject
#include "mudObjects/players.hpp"    // for Player
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for lowercize
#include "server.hpp"                // for Server, gServer
#include "stats.hpp"                 // for Stat
#include "threat.hpp"                // for ThreatTable, ThreatEntry, ThreatMap

//################################################################################
//#       Threat Table
//################################################################################
std::ostream& operator<<(std::ostream& out, const ThreatTable* table) {
    if(table)
        out << (*table);
    return(out);
}
std::ostream& operator<<(std::ostream& out, const ThreatTable& table) {
    for(ThreatEntry* threat : table.threatSet) {
        out << threat << std::endl;
    }
    return(out);
}

ThreatTable::ThreatTable(Creature *cParent) {
    setParent(cParent);
    totalThreat = 0;
}

ThreatTable::~ThreatTable() {
    clear();
}


// Clear the threat table
void ThreatTable::clear() {
    for(const ThreatMap::value_type& p : threatMap) {
        delete p.second;
    }
    threatMap.clear();
    threatSet.clear();
}

// Returns the size of the threat table
ThreatMap::size_type ThreatTable::size() const {
    return(threatMap.size());
}

long ThreatTable::getTotalThreat() {
    return(totalThreat);
}

long ThreatTable::getThreat(const std::shared_ptr<Creature>& target) {
    auto it = threatMap.find(target->getId());
    if(it != threatMap.end()) {
        return((*it).second->getThreatValue());
    }
    return(0);
}

//*********************************************************************
//* AdjustThreat
//*********************************************************************

// Adjusts threat for the given target
// If target already exists in the list update it, otherwise add it
// Maintains a map of threat sorted by ID and a multiset sorted by threat amount

long ThreatTable::adjustThreat(const std::shared_ptr<Creature>& target, long modAmt, double threatFactor) {
    if(target->getId() == myParent->getId()) {
        std::clog << "Attempted to add " << target->getName()  << " to their own threat list!" << std::endl;
        return(0);
    }
    bool newThreat = false;
    ThreatEntry* threat;

    ThreatSet::iterator it;

    // Find the place in the threat list it is, or would be
    auto mLb = threatMap.lower_bound(target->getId());

    // If it's there, update it
    if(mLb != threatMap.end() && !(threatMap.key_comp()(target->getId(), mLb->first))) {
        threat = mLb->second;
        it = removeFromSet(threat);
    } else {
        // Otherwise make a new one and insert it into the position we just found
        threat = new ThreatEntry(target->getId());
        threatMap.insert(mLb, ThreatMap::value_type(target->getId(), threat));
        newThreat = true;
    }

    // Adjust the threat
    long endThreat = threat->adjustThreat((long)(modAmt*threatFactor));
    threat->adjustContribution(modAmt);

    //std::clog << "Added Threat: " << ((long)(modAmt*threatFactor)) << " and Contribution: " << modAmt << std::endl;
    // Insert the threat into the set, if it's not a new threat use the hint from above
    if(newThreat)
        threatSet.insert(threat);
    else
        threatSet.insert(it, threat);

    return(endThreat);
}
bool ThreatTable::isEnemy(const Creature *target) const {
    if(!target) return false;
    auto it = threatMap.find(target->getId());

    return !(it == threatMap.end());
}

bool ThreatTable::isEnemy(const std::shared_ptr<const Creature> &target) const {
    return isEnemy(target.get());
}

bool ThreatTable::hasEnemy() const {
    return(!threatMap.empty());
}

//*********************************************************************
//* RemoveFromSet
//*********************************************************************

// Removes the given threat from the threatSet

ThreatSet::iterator ThreatTable::removeFromSet(ThreatEntry* threat) {
    ThreatSet::iterator it;

    // Remove the threat from set because we're going to be modifying the key value
    auto p = threatSet.equal_range(threat);

    // We use the std::find algorithm because we want the exact threat
    // not an equivalent threat.  Narrow it down first using equal range so we don't
    // search the entire threatSet
    it = std::find(p.first, p.second, threat);

    // We should have found the same threat
    assert((*it) == threat);
    assert(it != p.second);
    // Increment the iterator so it's still valid, it may be used as a hint later
    threatSet.erase(it++);
    return(it);
}

//*********************************************************************
//* RemoveThreat
//*********************************************************************

// Completely removes the target from this threat list and returns the
// amount of contribution they had before removal

long ThreatTable::removeThreat(const std::string &pUid) {
    long toReturn = 0;
    auto mIt = threatMap.find(pUid);
    if(mIt == threatMap.end())
        return(toReturn);
    ThreatEntry* threat = (*mIt).second;
    toReturn = threat->getContributionValue();
    removeFromSet(threat);
    threatMap.erase(mIt);
    delete threat;
    return(toReturn);
}
long ThreatTable::removeThreat(const std::shared_ptr<Creature>& target) {
    return(removeThreat(target->getId()));
}

//*********************************************************************
//* GetTarget
//*********************************************************************
// Returns the Creature to attack based on threat and intelligent of
// the parent.  Returns null if it's not mad at anybody.

std::shared_ptr<Creature> ThreatTable::getTarget(bool sameRoom) {
    if(threatSet.empty()) {
        return(nullptr);
    }

    if(myParent == nullptr) {
        std::cerr << "Error: Null parent in ThreatTable::getTarget()" << std::endl;
        return(nullptr);
    }

    std::shared_ptr<Creature> toReturn = nullptr;
    std::shared_ptr<Creature> crt;
    auto myRoom = myParent->getRoomParent();

    ThreatSet::reverse_iterator it;
    for(it = threatSet.rbegin() ; it != threatSet.rend() ; ) {
        std::string uId = (*it++)->getUid();
        crt = gServer->lookupCrtId(uId);
        if(!crt && uId.at(0) == 'M') {
            // If we're a monster and the server hasn't heard of them, they're either a pet
            // who has logged off, or a dead monster, either way remove them.
            removeThreat(uId);
            it = threatSet.rbegin();
            continue;
        }
        // If we're looking for someone who isn't in the same room, we don't care if we can see them
        // however if we want the target in the current room, we must be able to see them!
        if(!crt || (sameRoom && (crt->getRoomParent() != myRoom || !myParent->canSee(crt)))) continue;

        // The highest threat creature (in the same room if sameRoom is true)
        toReturn = crt;
        break;
    }

    return(toReturn);
}

void ThreatTable::setParent(Creature *cParent) {
    myParent = cParent;
}

//################################################################################
//#       Threat Entry
//################################################################################

ThreatEntry::ThreatEntry(std::string_view pUid) {
    threatValue = 0;
    contributionValue = 0;
    uId = pUid;
    lastMod = time(nullptr);
}

std::ostream& operator<<(std::ostream& out, const ThreatEntry& threat) {
    std::shared_ptr<MudObject> mo = gServer->lookupCrtId(threat.getUid());

    out << "ID: " << threat.uId;
    if(mo)
        out << " (" << mo->getName() << ")";
    else
        out << " (Unknown Target)";

    out << " T: " << threat.threatValue << " C: " << threat.contributionValue;
    return(out);
}

std::ostream& operator<<(std::ostream& out, const ThreatEntry* threat) {
    if(threat)
        out << *threat;

    return(out);
}

long ThreatEntry::getThreatValue() const {
    return threatValue;
}
long ThreatEntry::getContributionValue() const {
    return contributionValue;
}

long ThreatEntry::adjustThreat(long modAmt) {
    threatValue += modAmt;
    lastMod = time(nullptr);
    return(threatValue);
}
long ThreatEntry::adjustContribution(long modAmt) {
    contributionValue += modAmt;
    lastMod = time(nullptr);
    return(contributionValue);
}
// ThreatPtr comparison
bool ThreatPtrLess::operator()(const ThreatEntry* lhs, const ThreatEntry* rhs) const {
    return *lhs < *rhs;
}

bool ThreatEntry::operator< (const ThreatEntry& t) const {
    return(this->threatValue < t.threatValue);
}

const std::string & ThreatEntry::getUid() const {
    return(uId);
}
//*********************************************************************
//                          doHeal
//*********************************************************************
// Heal a target and dish out threat as needed!

unsigned int Creature::doHeal(const std::shared_ptr<Creature>& target, int amt, double threatFactor) {
    auto cThis = Containable::downcasted_shared_from_this<Creature>();
    if(threatFactor < 0.0)
        threatFactor = 1.0;

    unsigned int healed = target->hp.increase(amt);

    // If the target is a player/pet and they're in combat then this counts towards the damage done/hatred on that monster
    if((target->isPlayer() || target->isPet()) && target->inCombat(false)) {
        for(const auto& mons : getRoomParent()->monsters) {
            if(mons->isEnemy(target)) {
                // If we're not on the enemy list, put us on at the end
                if(!mons->isEnemy(cThis)) {
                    mons->addEnemy(cThis);
                    *this << ColorOn << setf(CAP) << "^R " << mons << " gets angry at you!^x\n" << ColorOff;
                }
                // Add the amount of healing threat done to effort done
                mons->adjustThreat(cThis, healed, threatFactor);
            }
        }
    }
    return(healed);
}




//################################################################################
// Targeting Functions
//################################################################################

void Creature::checkTarget(const std::shared_ptr<Creature>& toTarget) {
    if(isPlayer() && !flagIsSet(P_NO_AUTO_TARGET) && getTarget() == nullptr) {
        addTarget(toTarget);
    }
}

//*********************************************************************
//                          addTarget
//*********************************************************************

std::shared_ptr<Creature> Creature::addTarget(const std::shared_ptr<Creature>& toTarget) {
    if(!toTarget)
        return(nullptr);

    // We've already got them targetted!
    auto lockedTarget = myTarget.lock();
    if(toTarget == lockedTarget)
        return(lockedTarget);

    clearTarget();

    toTarget->addTargetingThis(Containable::downcasted_shared_from_this<Creature>());
    myTarget = toTarget;

    std::shared_ptr<Player> ply = getAsPlayer();
    if(ply) {
        ply->printColor("You are now targeting %s.\n", toTarget->getCName());
    }

    return(lockedTarget);

}

//*********************************************************************
//                          addTargetingThis
//*********************************************************************

void Creature::addTargetingThis(const std::shared_ptr<Creature>& targeter) {
    std::shared_ptr<Player> ply = getAsPlayer();
    if(ply) {
        ply->printColor("%s is now targeting you!\n", targeter->getCName());
    }
    targetingThis.push_back(targeter);
}

//*********************************************************************
//                          clearTarget
//*********************************************************************

void Creature::clearTarget(bool clearTargetsList) {
    auto lockedTarget = myTarget.lock();
    if(isPlayer()) {
        if(lockedTarget)
            printColor("You are no longer targeting %s!\n", lockedTarget->getCName());
        else
            printColor("You no longer have a target!\n");
    }

    if(lockedTarget && clearTargetsList)
        lockedTarget->clearTargetingThis(this);

    myTarget.reset();
}

//*********************************************************************
//                          clearTargetingThis
//*********************************************************************

void Creature::clearTargetingThis(Creature *targeter) {
    if(isPlayer()) {
        printColor("%s is no longer targeting you!\n", targeter->getCName());
    }

    targetingThis.remove_if([&targeter](const std::weak_ptr<Creature>& ptr) {
        return ptr.lock().get() == targeter;
    });
}

//*********************************************************************
//                          cmdAssist
//*********************************************************************

int cmdAssist(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("Who would you like to assist?\n");

    }
    std::shared_ptr<Player> toAssist = player->getParent()->findPlayer(player, cmnd);
    if(!toAssist) {
        player->print("You don't see that person here.\n");
        return(0);
    }
    *player << "You assist " << setf(CAP) << toAssist << "!\n";

    if(!toAssist->flagIsSet(P_COMPACT))
        *toAssist << setf(CAP) << player << " just assisted you!\n";
    
    player->clearTarget();
    player->addTarget(toAssist->getTarget());

    return(0);
}

//*********************************************************************
//                          cmdTarget
//*********************************************************************

int cmdTarget(const std::shared_ptr<Player>& player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("You are targeting: ");
        if(auto target = player->myTarget.lock())
            player->print("%s\n", target->getCName());
        else
            player->print("No-one!\n");

        if(player->isCt()) {
            player->print("People targeting you: ");
            int numTargets = 0;
            for(auto it = player->targetingThis.begin() ; it != player->targetingThis.end() ; ) {
                if(auto targetter = it->lock()) {
                    if (numTargets++ != 0)
                        player->print(", ");
                    player->print("%s", targetter->getCName());
                    it++;
                } else {
                    it = player->targetingThis.erase(it);
                }
            }
            if(numTargets == 0)
                player->print("Nobody!");
            player->print("\n");
        }
        return(0);
    }

    if(!strcasecmp(cmnd->str[1], "-c")) {
        player->print("Clearing target.\n");
        player->clearTarget();
        return(0);
    }

    lowercize(cmnd->str[1], 1);

    std::shared_ptr<Creature> toTarget = player->getParent()->findCreature(player, cmnd);
    if(!toTarget) {
        player->print("You don't see that here.\n");
        return(0);
    }
    player->addTarget(toTarget);

    return(0);
}

//*********************************************************************
//                          getTarget
//*********************************************************************

std::shared_ptr<Creature> Creature::getTarget() {
    return(myTarget.lock());
}

//*********************************************************************
//                          hasAttackableTarget
//*********************************************************************

bool Creature::hasAttackableTarget() {
    return(getTarget() && getTarget().get() != this && inSameRoom(getTarget()) && canSee(getTarget()));
}


//*********************************************************************
//                          isAttackingTarget
//*********************************************************************

bool Creature::isAttackingTarget() {
    std::shared_ptr<Creature> target = getTarget();
    if(!target)
        return(false);

    std::shared_ptr<Monster>  mTarget = target->getAsMonster();

    // Player auto combat only works vs monsters!
    if(!mTarget)
        return(false);

    return(mTarget->isEnemy(Containable::downcasted_shared_from_this<Creature>()));
}

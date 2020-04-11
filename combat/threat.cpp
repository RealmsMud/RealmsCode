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
 *  Copyright (C) 2007-2020 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */
#include <cassert>                // for assert
#include <cstring>                // for strcasecmp
#include <ctime>                  // for time
#include <functional>             // for less
#include <iostream>               // for operator<<, ostream, basic_ostream
#include <map>                    // for operator==, operator!=
#include <string>                 // for operator<<, char_traits, operator==

#include "bstring.hpp"            // for bstring
#include "cmd.hpp"                // for cmd
#include "commands.hpp"           // for cmdAssist, cmdTarget
#include "creatures.hpp"          // for Creature, Player, Monster
#include "flags.hpp"              // for P_COMPACT, P_NO_AUTO_TARGET
#include "mudObject.hpp"          // for MudObject
#include "os.hpp"                 // for ASSERTLOG
#include "proto.hpp"              // for lowercize
#include "rooms.hpp"              // for BaseRoom
#include "server.hpp"             // for Server, gServer
#include "threat.hpp"             // for ThreatTable, ThreatEntry, ThreatMap


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

ThreatTable::ThreatTable(Creature* pParent) {
    setParent(pParent);
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
ThreatMap::size_type ThreatTable::size() {
    return(threatMap.size());
}

long ThreatTable::getTotalThreat() {
    return(totalThreat);
}

long ThreatTable::getThreat(Creature* target) {
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

long ThreatTable::adjustThreat(Creature* target, long modAmt, double threatFactor) {
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

bool ThreatTable::isEnemy(const Creature *target) {
    if(!target) {
        return(false);
    }
    auto it = threatMap.find(target->getId());

    return !(it == threatMap.end());
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

long ThreatTable::removeThreat(const bstring& pUid) {
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
long ThreatTable::removeThreat(Creature* target) {
    return(removeThreat(target->getId()));
}

//*********************************************************************
//* GetTarget
//*********************************************************************
// Returns the Creature to attack based on threat and intelligent of
// the parent.  Returns null if it's not mad at anybody.

Creature* ThreatTable::getTarget(bool sameRoom) {
    if(threatSet.empty()) {
        return(nullptr);
    }

    if(myParent == nullptr) {
        std::cerr << "Error: Null parent in ThreatTable::getTarget()" << std::endl;
        return(nullptr);
    }

    Creature* toReturn = nullptr;
    Creature* crt = nullptr;

    ThreatSet::reverse_iterator it;
    for(it = threatSet.rbegin() ; it != threatSet.rend() ; ) {
        bstring uId = (*it++)->getUid();
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
        if(!crt || (sameRoom && (crt->getRoomParent() != myParent->getRoomParent() || !myParent->canSee(crt)))) continue;

        // The highest threat creature (in the same room if sameRoom is true)
        toReturn = crt;
        break;
    }

    return(toReturn);
}

void ThreatTable::setParent(Creature* pParent) {
    myParent = pParent;
}

//################################################################################
//#       Threat Entry
//################################################################################

ThreatEntry::ThreatEntry(const bstring& pUid) {
    threatValue = 0;
    contributionValue = 0;
    uId = pUid;
    lastMod = time(nullptr);
}

std::ostream& operator<<(std::ostream& out, const ThreatEntry& threat) {
    MudObject* mo = gServer->lookupCrtId(threat.getUid());

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

long ThreatEntry::getThreatValue() {
    return threatValue;
}
long ThreatEntry::getContributionValue() {
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

const bstring& ThreatEntry::getUid() const {
    return(uId);
}
//*********************************************************************
//                          doHeal
//*********************************************************************
// Heal a target and dish out threat as needed!

int Creature::doHeal(Creature* target, int amt, double threatFactor) {
    if(threatFactor < 0.0)
        threatFactor = 1.0;

    int healed = target->hp.increase(amt);

    // If the target is a player/pet and they're in combat
    // then this counts towards the damage done/hatred on that monster
    if((target->isPlayer() || target->isPet()) && target->inCombat(false)) {
        for(Monster* mons : getRoomParent()->monsters) {
            if(mons->isEnemy(target)) {
                // If we're not on the enemy list, put us on at the end
                if(!mons->isEnemy(this)) {
                    mons->addEnemy(this);
                    this->printColor("^R%M gets angry at you!^x\n", mons);
                }
                // Add the amount of healing threat done to effort done
                mons->adjustThreat(this, healed, threatFactor);
            }
        }
    }
    return(healed);
}




//################################################################################
// Targeting Functions
//################################################################################

void Creature::checkTarget(Creature* toTarget) {
    if(isPlayer() && !flagIsSet(P_NO_AUTO_TARGET) && getTarget() == nullptr) {
        addTarget(toTarget);
    }
}

//*********************************************************************
//                          addTarget
//*********************************************************************

Creature* Creature::addTarget(Creature* toTarget) {
    if(!toTarget)
        return(nullptr);

    // We've already got them targetted!
    if(toTarget == myTarget)
        return(myTarget);

    clearTarget();

    toTarget->addTargetingThis(this);
    myTarget = toTarget;

    Player* ply = getAsPlayer();
    if(ply) {
        ply->printColor("You are now targeting %s.\n", myTarget->getCName());
    }

    return(myTarget);

}

//*********************************************************************
//                          addTargetingThis
//*********************************************************************

void Creature::addTargetingThis(Creature* targeter) {
    ASSERTLOG(targeter);

    Player* ply = getAsPlayer();
    if(ply) {
        ply->printColor("%s is now targeting you!\n", targeter->getCName());
    }
    targetingThis.push_back(targeter);
}

//*********************************************************************
//                          clearTarget
//*********************************************************************

void Creature::clearTarget(bool clearTargetsList) {
    if(!myTarget)
        return;

    Player* ply = getAsPlayer();
    if(ply) {
        ply->printColor("You are no longer targeting %s!\n", myTarget->getCName());
    }

    if(clearTargetsList)
        myTarget->clearTargetingThis(this);

    myTarget = nullptr;
}

//*********************************************************************
//                          clearTargetingThis
//*********************************************************************

void Creature::clearTargetingThis(Creature* targeter) {
    ASSERTLOG(targeter);

    Player* ply = getAsPlayer();
    if(ply) {
        ply->printColor("%s is no longer targeting you!\n", targeter->getCName());
    }

    targetingThis.remove(targeter);
}

//*********************************************************************
//                          cmdAssist
//*********************************************************************

int cmdAssist(Player* player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("Who would you like to assist?\n");

    }
    Player* toAssist = player->getParent()->findPlayer(player, cmnd);
    if(!toAssist) {
        player->print("You don't see that person here.\n");
        return(0);
    }
    player->print("You assist %M!\n", toAssist);

    if(!toAssist->flagIsSet(P_COMPACT))
        toAssist->print("%M just assisted you!\n", player);
    
    player->clearTarget();
    player->addTarget(toAssist->getTarget());

    return(0);
}

//*********************************************************************
//                          cmdTarget
//*********************************************************************

int cmdTarget(Player* player, cmd* cmnd) {
    if(cmnd->num < 2) {
        player->print("You are targeting: ");
        if(player->myTarget)
            player->print("%s\n", player->myTarget->getCName());
        else
            player->print("No-one!\n");

        if(player->isCt()) {
            player->print("People targeting you: ");
            int numTargets = 0;
            for(Creature* targetter : player->targetingThis) {
                if(numTargets++ != 0)
                    player->print(", ");
                player->print("%s", targetter->getCName());
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

    Creature* toTarget = player->getParent()->findCreature(player, cmnd);
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

Creature* Creature::getTarget() {
    return(myTarget);
}

//*********************************************************************
//                          hasAttackableTarget
//*********************************************************************

bool Creature::hasAttackableTarget() {
    return(getTarget() && getTarget() != this && inSameRoom(getTarget()) && canSee(getTarget()));
}


//*********************************************************************
//                          isAttackingTarget
//*********************************************************************

bool Creature::isAttackingTarget() {
    Creature* target = getTarget();
    if(!target)
        return(false);

    Monster* mTarget = target->getAsMonster();

    // Player auto combat only works vs monsters!
    if(!mTarget)
        return(false);

    return(mTarget->isEnemy(this));
}

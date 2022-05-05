/*
 * pets.cpp
 *   Functions dealing with pets
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


#include <algorithm>                 // for find
#include <iomanip>                   // for operator<<, setw
#include <list>                      // for list<>::iterator, operator==
#include <ostream>                   // for operator<<, basic_ostream, ostri...
#include <string>                    // for char_traits, operator<<, basic_s...
#include <utility>

#include "flags.hpp"                 // for M_PET, P_NO_EXTRA_COLOR
#include "group.hpp"                 // for Group
#include "mudObjects/creatures.hpp"  // for Creature, PetList
#include "mudObjects/monsters.hpp"   // for Monster
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for broadcast, isMatch
#include "stats.hpp"                 // for Stat

void Creature::addPet(std::shared_ptr<Monster>  newPet, bool setPetFlag) {
    if(newPet->getMaster())
        return;

    if(setPetFlag)
        newPet->setFlag(M_PET);
    newPet->setMaster(Containable::downcasted_shared_from_this<Creature>());
    pets.push_back(newPet);
    if(getGroup())
        getGroup()->add(newPet);
}
void Creature::delPet(const std::shared_ptr<Monster>&  toDel) {
    auto it = std::find(pets.begin(), pets.end(), toDel);
    if(it != pets.end()) {
        pets.erase(it);
    }

}
std::shared_ptr<Monster>  Creature::findPet(const std::shared_ptr<Monster>&  toFind) {
    auto it = std::find(pets.begin(), pets.end(), toFind);
    if(it != pets.end())
        return((*it));
    return(nullptr);
}

bool Creature::hasPet() const {
    if(this == nullptr)
        return false;

    return(!pets.empty());
}

void Monster::setMaster(std::shared_ptr<Creature> pMaster) {
    myMaster = std::move(pMaster);
}

std::shared_ptr<Creature> Monster::getMaster() const {
    return(myMaster);
}

std::shared_ptr<Monster>  Creature::findPet(const std::string& pName, int pNum) {
    int match = 0;
    for(const auto& pet : pets) {
        if(isMatch(Containable::downcasted_shared_from_this<Creature>(), pet, pName, false, false)) {
            match++;
            if(match == pNum) {
                return(pet);
            }
        }
    }
    return(nullptr);
}

//*********************************************************************
//                      isPet
//*********************************************************************

bool Creature::isPet() const {
    if(isPlayer())
        return(false);
    return(flagIsSet(M_PET) && getAsConstMonster()->getMaster());
}

void Creature::dismissPet(const std::shared_ptr<Monster>&  pet) {
    if(pet->getMaster().get() != this)
        return;

    print("You dismiss %N.\n", pet.get());
    broadcast(getSock(), getRoomParent(), "%M dismisses %N.", this, pet.get());

    if(pet->isUndead())
        broadcast(nullptr, getRoomParent(), "%M wanders away.", pet.get());
    else
        broadcast(nullptr, getRoomParent(), "%M fades away.", pet.get());
    pet->die(Containable::downcasted_shared_from_this<Creature>());

}
void Creature::dismissAll() {
    // We use this instead of for() because dismissPet will remove it from the list and invalidate the iterators
    PetList::iterator it;
    for(it = pets.begin() ; it != pets.end() ; ) {
        std::shared_ptr<Monster>  pet = (*it++);
        dismissPet(pet);
    }
}
void Creature::displayPets() {
    std::ostringstream oStr;
    oStr << "Your pet" << (pets.size() > 1 ? "s" : "") << ":" << std::endl;
    for(const auto& pet : pets) {

        oStr << pet->getName();

        oStr << " - " << (pet->hp.getCur() < pet->hp.getMax() && !pFlagIsSet(P_NO_EXTRA_COLOR) ? "^R" : "")
             << std::setw(3) << pet->hp.getCur() << "^x/" << std::setw(3) << pet->hp.getMax()
             << " Hp - " << std::setw(3) << pet->mp.getCur() << "/" << std::setw(3)
             << pet->mp.getMax() << " Mp";
        oStr << std::endl;
    }
    printColor("%s", oStr.str().c_str());
}

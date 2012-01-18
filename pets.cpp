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
 *  Creative Commons - Attribution - Non Commercial - Share Alike 3.0 License
 *    http://creativecommons.org/licenses/by-nc-sa/3.0/
 *
 *  Copyright (C) 2007-2012 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "mud.h"

void Creature::addPet(Monster* newPet, bool setPetFlag) {
    if(newPet->getMaster())
        return;

    if(setPetFlag)
        newPet->setFlag(M_PET);
    newPet->setMaster(this);
    pets.push_back(newPet);
    if(getGroup())
    	getGroup()->add(newPet);
}
void Creature::delPet(Monster* toDel) {
    PetList::iterator it = std::find(pets.begin(), pets.end(), toDel);
    if(it != pets.end()) {
        pets.erase(it);
    }

}
Monster* Creature::findPet(Monster* toFind) {
    PetList::iterator it = std::find(pets.begin(), pets.end(), toFind);
    if(it != pets.end())
        return((*it));
    return(NULL);
}

bool Creature::hasPet() const {
    return(!pets.empty());
}

void Monster::setMaster(Creature* pMaster) {
    myMaster = pMaster;
}

Creature* Monster::getMaster() const {
    if(!this)
        return(NULL);

    return(myMaster);
}

template<class Type>
Type findCreature(std::list<Type>& list, bstring pName, int pNum, bool exactMatch) {
    if(pName.empty())
        return(NULL);

    int match = 0;
    for(Type target : list) {
        if(exactMatch) {
            if(!pName.equals(target->getName())) {
                match++;
                if(match == pNum) {
                    return(target);
                }
            }
        } else {
            if(keyTxtEqual(target, pName.c_str())) {
                match++;
                if(match == pNum) {
                    return(target);
                }
            }
        }
    }
    return(NULL);

}

Monster* Creature::findPet(bstring pName, int pNum) {
    return(findCreature<Monster*>(pets, pName, pNum, false));
}

//*********************************************************************
//                      isPet
//*********************************************************************

bool Creature::isPet() const {
    if(isPlayer())
        return(false);
    return(flagIsSet(M_PET) && getConstMonster()->getMaster());
}

void Creature::dismissPet(Monster* pet) {
	if(pet->getMaster() != this)
		return;

	print("You dismiss %N.\n", pet);
	broadcast(getSock(), getRoom(), "%M dismisses %N.", this, pet);

	if(pet->isUndead())
		broadcast(NULL, getRoom(), "%M wanders away.", pet);
	else
		broadcast(NULL, getRoom(), "%M fades away.", pet);
	pet->die(this);
	gServer->delActive(pet);

}
void Creature::dismissAll() {
	// We use this instead of for() because dismissPet will remove it from the list and invalidate the iterators
	PetList::iterator it;
	for(it = pets.begin() ; it != pets.end() ; ) {
		Monster* pet = (*it++);
		dismissPet(pet);
	}
}

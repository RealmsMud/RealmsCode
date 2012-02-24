/* 
 * alchemy.cpp
 *   Alchmey classes, functions, and other handlers
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

#include "mud.h"
#include "craft.h"
#include "factions.h"
#include "commands.h"
#include "unique.h"
#include "alchemy.h"

#include <sstream>
#include <iomanip>
#include <locale>


//*********************************************************************
//						getDisplayString
//*********************************************************************

bstring AlchemyInfo::getDisplayString() {
	std::ostringstream displayStr;

	displayStr << "^W" << std::setw(25) << name << "^x - " << (positive ? "yes" : " ^rno^x") << " - " << std::setw(-15);
	
	if(action == "python")
		displayStr << "^c";

	displayStr << action << "^x";

	return(displayStr.str());
}

//*********************************************************************
//						clearAlchemy
//*********************************************************************

bool Config::clearAlchemy() {
	for(AlchemyInfo* alcInfo : alchemy) {
		if(alcInfo)
			delete alcInfo;
	}
	alchemy.clear();
	return(true);
}


//*********************************************************************
//						getAlchemyInfo
//*********************************************************************

const AlchemyInfo *Config::getAlchemyInfo(bstring effect) const {
	for(AlchemyInfo* alcInfo : alchemy) {
		if(alcInfo && alcInfo->name == effect)
			return alcInfo;
	}
	return(NULL);
}

//*********************************************************************
//						Alchemy
//*********************************************************************

namespace Alchemy {
	int getVisibleEffects(int alchemySkill) {
		int visibleEffects = 0;
		
		if(alchemySkill >= 75)
			visibleEffects = 4;
		else if(alchemySkill >= 50)
			visibleEffects = 3;
		else if(alchemySkill >= 25)
			visibleEffects = 2;
		else if(alchemySkill >= 1)
			visibleEffects = 1;
		else
			visibleEffects = 0;
		
		return(visibleEffects);		
	}
	long MAX_ALCHEMY_DURATION = 3900;
	long getMaximumDuration() {
		return(MAX_ALCHEMY_DURATION);
	}
}

//*********************************************************************
//						AlchemyEffect
//*********************************************************************

AlchemyEffect::AlchemyEffect() {
	duration = strength = 0;
	quality = 1;
}

AlchemyEffect::AlchemyEffect(const AlchemyEffect &ae) {
	effect = ae.effect;
	duration = ae.duration;
	strength = ae.strength;
	quality = ae.quality;
}

//*********************************************************************
//						getEffect
//*********************************************************************

const bstring& AlchemyEffect::getEffect() const {
	return(effect);
}

//*********************************************************************
//						getDuration
//*********************************************************************

long AlchemyEffect::getDuration() const {
	return(duration);
}

//*********************************************************************
//						getStrength
//*********************************************************************

short AlchemyEffect::getStrength() const {
	return(strength);
}

//*********************************************************************
//						getQuality
//*********************************************************************

short AlchemyEffect::getQuality() const {
	return(quality);
}

//*********************************************************************
//						setDuration
//*********************************************************************

void AlchemyEffect::setDuration(const long newDuration) {
	duration = tMIN<long>(tMAX<long>(newDuration,0), Alchemy::getMaximumDuration());
}

//*********************************************************************
//						combineWith
//*********************************************************************
// Adjusts to the average of the two effects, used to form a potion

void AlchemyEffect::combineWith(const AlchemyEffect& ae) {
	quality = (int)(((float)(ae.quality + quality))/2.0);
}


//*********************************************************************
//						showAlchemyEffects
//*********************************************************************
// NOTE: A null creature is perfectly valid, so handle it properly

bstring Object::showAlchemyEffects(Creature *creature) {
	bstring toReturn;

	if(!alchemyEffects.empty() && (!creature || creature->isCt() || creature->knowsSkill("alchemy"))) {
		// Find out how many effects to show this person
		int skillLevel = 0, visibleEffects = 1, shown = 0;


		if(!creature || creature->isCt())
			skillLevel = 100;
		else
			skillLevel = (int)(creature->getSkillGained("alchemy")/3);

		visibleEffects = Alchemy::getVisibleEffects(skillLevel);

		std::ostringstream outStr;

		outStr << "Alchemy Effects:\n";
		for(std::pair<int, AlchemyEffect> p : alchemyEffects) {
			outStr << p.first << ") " << p.second.getEffect();
			if(!creature || creature->isDm()) {
				// Potions have duration/strength, herbs have quality
				if(type == POTION)
					outStr << " D: " << p.second.getDuration() << " S: " << p.second.getStrength();
				else
					outStr << " Q: " << p.second.getQuality();
			}


			outStr << "\n";

			// Have we shown enough effects yet?
			if(type != POTION && ++shown == visibleEffects)
				break;
		}

		toReturn = outStr.str();

	}

	return(toReturn);
}


//*********************************************************************
//						cmdBrew
//*********************************************************************

int cmdBrew(Player* player, cmd* cmnd) {
	BaseRoom* room = player->getRoomParent();

	if(!player->knowsSkill("alchemy")) {
		player->print("You have no idea how to brew potions!\n");
		return(0);
	}

	// Keep track of the player's alchemy skill level
	int skillLevel = (int)(player->getSkillGained("alchemy"));

	// Handle recpies later, for now a herb container is the target
	if(cmnd->num < 2) {
		player->print("Well, what would you like to brew?\n");
		return(0);
	}

	Object* mortar=0;	// Our Mortar and Pestle
	Object* retort=0;
	Object* alembic=0;

	double retortQuality = 0;
	double mortarQuality = 0;

	Object* herb=0;		// The herb we're looking at

	// Lets find our mortar.  First check inventory
	mortar = findObject(player, player->first_obj, cmnd);

	// Second check the room
	if(!mortar)
		mortar = findObject(player, room->first_obj, cmnd, 1);

	if(!mortar) {
		player->print("You don't have that.\n");
		return(0);
	}


	if(mortar->getType() != CONTAINER || mortar->getSubType() != "mortar") {
		player->print("That isn't a mortar and pestle!\n");
		return(0);
	}

	if(!mortar->first_obj) {
		player->print("But it's empty, what do you want to brew?\n");
		return(0);
	}

	if(mortar->getShotsCur() < 1) {
		player->print("You don't have enough herbs in there to brew anything.\n");
		return(0);
	} else if(mortar->getShotsCur() < 2 && skillLevel < 300) {
		player->print("You need at least two herbs to brew something.");
		return(0);
	}
	mortarQuality = mortar->getQuality()/10.0;

	// We'll be combining multiple herbs into one potion
	// Skill level can be 1-100
	otag* op = 0;
	std::map<bstring, AlchemyEffect> effects;
	if(mortar->getShotsCur() >= 2) {
		std::map<bstring, int> effectCount;
		int visibleEffects = Alchemy::getVisibleEffects(skillLevel);

		// We want to look at the first 4 herbs in the mortar and get a list of effects and how many occurances of that
		// effect there are.  For any effect with 2 or more occurances, it'll get added to the final potion
		// If we have no effects with 2 or more occurances, we have a failed attempt to make a potion.
		op = mortar->first_obj;
		int numHerbs = 0;
		while(op) {
			herb = op->obj;

			int used = 0;
			for(std::pair<int, AlchemyEffect> p : herb->alchemyEffects) {

				bstring effect = p.second.getEffect();
				effectCount[effect]++;

				if(effects.find(effect) == effects.end()) {
					effects[effect] = p.second;
				} else {
					// The effect is based on the minimum strength in the herbs
					effects[effect].combineWith(p.second);
				}

				// We can only use visible effects
				if(++used == visibleEffects)
					break;
			}
			if(++numHerbs == 4)
				break;
			op = op->next_tag;
		} // end while

		for(std::pair<bstring, int> effectPair : effectCount) {
			if(effectPair.second > 1) {
				player->printColor("Using effect: ^Y%s^x.\n", effectPair.first.c_str());
			}
			else {
				effects.erase(effectPair.first);
				//player->print("Skipping effect: %s\n", effectPair.first.c_str());
			}
		} // end foreach
	}  // end if
	// We'll be using just one herb, so it'll be the first effect
	// To get here, we have to have 100 skill
	else if(mortar->getShotsCur() == 1) {
		AlchemyEffect &ae = mortar->first_obj->obj->alchemyEffects[1];
		effects[ae.getEffect()] = ae;
		player->printColor("Brewing a single effect potion: ^Y%s^x\n", ae.getEffect().c_str());
		return(0);
	}

	// We know we're making a potion now, so see if there's an alembic or a retort in the player's inventory
	// or in the room, use the most powerful one found
	op = player->first_obj;
	bool playerSearched = false;
	while(op) {
		if(op->obj->getSubType() == "retort") {
			if(!retort || op->obj->getQuality() > retort->getQuality()) {
				retort = op->obj;
			}
		} else if (op->obj->getSubType() == "alembic") {
			if(!alembic || op->obj->getQuality() > alembic->getQuality()) {
				alembic = op->obj;
			}
		}

		op = op->next_tag;
		if(!op && playerSearched == false) {
			// If we're at the end of the objects, and we haven't finish searching the player
			// we must have just finished searching the player, so set that to true and goto
			// the first object in the room.  If we have searched the player and the op is NULL
			// then we've searched everything so we'll just fall through
			playerSearched = true;
			op = player->getRoomParent()->first_obj;
		}
	}
	if(retort) {
		player->printColor("Using retort: ^Y%s^x\n", retort->getName());
		retortQuality = retort->getQuality()/10.0;
		// Modify stuff here based on the retort's quality
	}
	if(alembic) {
		player->printColor("Using alembic: ^Y%s^x\n", alembic->getName());
		// Modify stuff here based on the alembic's quality
	}

	// High quality mortars can add up to 25 effective skill
	skillLevel += (mortarQuality/4);



	long baseDur = (5.95387755 + (retortQuality*0.0612245))*skillLevel;

	Object* potion = Object::getNewPotion();
	;
	int i = 1;
	// Copy the alchemy effects to the potion
	for(std::pair<bstring, AlchemyEffect> aep : effects) {
		AlchemyEffect eff = aep.second;
		long duration = baseDur;

		const AlchemyInfo* alc = gConfig->getAlchemyInfo(eff.getEffect());
		if(alc) {
			// Adjust things based on the alchemy info
			//player->print("Found an alchemy Info!\n");
			//duration = alc->baseDuration;
		}
		eff.setDuration(duration);
		player->print("Effect: %s Duration: %d\n", eff.getEffect().c_str(), eff.getDuration());
		potion->addAlchemyEffect(i++, eff);
	}

	// TODO: Add a mechanism for naming an alchemy potion
	player->print("You have created %s!\n", potion->getName());
	potion->setDroppedBy(player, "Craft:Alchemy");
	player->addObj(potion);
	return(0);
}

//*********************************************************************
//						addAlchemyEffect
//*********************************************************************

bool Object::addAlchemyEffect(int num, const AlchemyEffect &ae) {
	if(num < 0)
		return false;
	alchemyEffects[num] = ae;

	return true;
}

//*********************************************************************
//						isAlchemyPotion
//*********************************************************************

bool Object::isAlchemyPotion() {
	return(type == POTION && alchemyEffects.size() > 0);
}

//*********************************************************************
//						consumeAlchemyPotion
//*********************************************************************

// Return: Was it consumed?
bool Object::consumeAlchemyPotion(Creature* consumer) {
	if(!isAlchemyPotion() || !consumer)
		return false;

	bool consumed = false;

	for(std::pair<int, AlchemyEffect> ae : alchemyEffects) {
		AlchemyEffect &eff = ae.second;
		const AlchemyInfo* alc = gConfig->getAlchemyInfo(eff.getEffect());
		if(!alc || alc->action == "effect") {
			// If one of the effects takes hold, the potion was consumed
			if(eff.apply(consumer))
				consumed = true;
		}
		else if(alc->action == "python") {
			if(gServer->runPython(alc->pythonScript, "", consumer, this))
				consumed = true;
		}
		else
			consumer->print("Unknown action: %s\n", alc->action.c_str());
	}

	return(consumed);
}

//*********************************************************************
//						apply
//*********************************************************************
// Apply this effect to the creature:
// Returns: false failure, true success

bool AlchemyEffect::apply(Creature* target) {
	// TODO: Check if it's an effect to add or something to apply immediately (death, heal, etc)
	bool add = true;

	if(effect == "poison" && target->immuneToPoison())
		add = false;
	else if(effect == "disease" && target->immuneToDisease())
		add = false;

	if(add)
		return(target->addEffect(effect, duration, strength));

	return(false);
}


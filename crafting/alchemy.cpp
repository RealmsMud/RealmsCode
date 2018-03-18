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
 *  GNU Affero General Public License v3 or later
 *  
 *  Copyright (C) 2007-2018 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include "alchemy.hpp"
#include "commands.hpp"
#include "config.hpp"
#include "craft.hpp"
#include "creatures.hpp"
#include "factions.hpp"
#include "mud.hpp"
#include "server.hpp"
#include "unique.hpp"

#include <sstream>
#include <iomanip>
#include <locale>

//########################################################################
//# AlchemyInfo
//########################################################################


//*********************************************************************
//                      getDisplayString
//*********************************************************************

bstring AlchemyInfo::getDisplayString() {
    std::ostringstream displayStr;

    displayStr << "^W" << std::setw(25) << name << "^x - " << (positive ? "yes" : " ^rno^x") << " - " << std::setw(-15);
    
    if(action == "python")
        displayStr << "^c";

    displayStr << action << "^x";

    return(displayStr.str());
}



const bstring& AlchemyInfo::getName() const {
    return(name);
}
const bstring& AlchemyInfo::getAction() const {
    return(action);
}
const bstring& AlchemyInfo::getPythonScript() const {
    return(pythonScript);
}
const bstring& AlchemyInfo::getPotionPrefix() const {
    return(potionPrefix);
}
const bstring& AlchemyInfo::getPotionDisplayName() const {
    return(potionDisplayName);
}

long AlchemyInfo::getBaseDuration() const {
    return(baseDuration);
}
short AlchemyInfo::getBaseStrength() const {
    return(baseStrength);
}
bool AlchemyInfo::isThrowable() const {
    return(throwable);
}
bool AlchemyInfo::isPositive() const {
    return(positive);
}


//*********************************************************************
//                      clearAlchemy
//*********************************************************************

bool Config::clearAlchemy() {
    for(std::pair<bstring, AlchemyInfo*> p : alchemy) {
        AlchemyInfo* alcInfo = p.second;
        if(alcInfo)
            delete alcInfo;
    }
    alchemy.clear();
    return(true);
}


//*********************************************************************
//                      getAlchemyInfo
//*********************************************************************

const AlchemyInfo *Config::getAlchemyInfo(bstring effect) const {
    AlchemyMap::const_iterator it = alchemy.find(effect);

    if(it != alchemy.end())
        return it->second;
    else
        return(nullptr);
}

//*********************************************************************
//                      Alchemy
//*********************************************************************

namespace Alchemy {
    const long MAX_ALCHEMY_DURATION = 3900;

    long getMaximumDuration() {
        return(MAX_ALCHEMY_DURATION);
    }

    bstring getEffectString(Object* obj, const bstring& effect) {
        if(!obj || effect.empty())
            return("*invalid*");
        return(obj->info.rstr() + "|" + effect);
    }

    int numEffectsVisisble(const int skillLevel) {
        if(skillLevel < 60)
            return(1);
        else if(skillLevel < 120)
            return(2);
        else if(skillLevel < 240)
            return(3);
        else
            return(4);
    }
}


//########################################################################
//# AlchemyEffect
//########################################################################

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
//                      getEffect
//*********************************************************************

const bstring& AlchemyEffect::getEffect() const {
    return(effect);
}

//*********************************************************************
//                      getDuration
//*********************************************************************

long AlchemyEffect::getDuration() const {
    return(duration);
}

//*********************************************************************
//                      getStrength
//*********************************************************************

short AlchemyEffect::getStrength() const {
    return(strength);
}

//*********************************************************************
//                      getQuality
//*********************************************************************

short AlchemyEffect::getQuality() const {
    return(quality);
}

//*********************************************************************
//                      setDuration
//*********************************************************************

void AlchemyEffect::setDuration(const long newDuration) {
    duration = MIN<long>(MAX<long>(newDuration,0), Alchemy::getMaximumDuration());
}

//*********************************************************************
//                      combineWith
//*********************************************************************
// Adjusts to the average of the two effects, used to form a potion

void AlchemyEffect::combineWith(const AlchemyEffect& ae) {
    quality = (short)(((float)(ae.quality + quality))/2.0);
}



//*********************************************************************
//                      alchemyEffectVisible
//*********************************************************************

bool Player::alchemyEffectVisible(Object* obj, const bstring effect) {
    if(!obj || effect.empty())
        return(false);

    // Anyone that can see alchemy effects can see potion effects
    if(obj->getType() == ObjectType::POTION)
        return(true);

    // Potions have been handled above, if we get here needs to be an herb
    if(obj->getType() != ObjectType::HERB)
        return(false);

    bstring effectStr = Alchemy::getEffectString(obj, effect);

    return((knownAlchemyEffects.find(effectStr) != knownAlchemyEffects.end()));

}

//*********************************************************************
//                      learnAlchemyEffect
//*********************************************************************

bool Player::learnAlchemyEffect(Object* obj, const bstring effect) {
    if(!obj || effect.empty())
        return(false);

    bstring effectStr = Alchemy::getEffectString(obj, effect);
    if(knownAlchemyEffects.find(effectStr) == knownAlchemyEffects.end()) {
        *this << ColorOn << "You have discovered a new alchemy effect: ^W" << obj->getName() << "^x has the effect: ^W" << effect << "^x\n" << ColorOff;
        knownAlchemyEffects[effectStr] = true;
        return(true);
    }

    return(false);
}

//*********************************************************************
//                      showAlchemyEffects
//*********************************************************************
// NOTE: A null player is perfectly valid, so handle it properly

bstring Object::showAlchemyEffects(Player *player) {
    bstring toReturn;
    int skillLevel=0, numVisible=0;
    bool isct = false, visible=false;

    if(player) {
        if(player->isCt())
            isct = true;
        if(player->knowsSkill("alchemy") || isct)
            visible = true;
        skillLevel = static_cast<int>(player->getTradeSkillGained("alchemy"));
    } else if(!player) {
        isct = true;
        visible = true;
        skillLevel = 300;  // TODO: Constant
    }

    numVisible = Alchemy::numEffectsVisisble(skillLevel);
    if(visible && !alchemyEffects.empty()) {
        int n=0;
        bool known;
        std::ostringstream outStr;

        outStr << "Alchemy Effects:\n";
        for(auto& p : alchemyEffects) {
            known = false;
            n++;

            outStr << p.first << ") ";

            if(n > numVisible && !isct)
                known = false;
            else if(player && player->alchemyEffectVisible(this, p.second.getEffect()))
                known = true;

            if(!known && isct) outStr << "(*) ";

            if(known || isct)
                outStr << p.second.getEffect();
            else
                outStr << "??? ";

            if(isct) {
                // Potions have duration/strength, herbs have quality
                if(type == ObjectType::POTION)
                    outStr << bstring(" D: ") << p.second.getDuration() << " S: " << p.second.getStrength();
                else
                    outStr << bstring(" Q: ") << p.second.getQuality();
            }
            outStr << "\n";
        }

        toReturn = outStr.str();

    }

    return(toReturn);
}

//**********************************************************************
//                      display
//**********************************************************************
std::ostream& operator<<(std::ostream& out, AlchemyEffect& effect) {
    out << effect.getEffect();
    return(out);
}

std::ostream& operator<<(std::ostream& out, AlchemyEffect* effect) {
    if (effect)
        out << *effect;

    return (out);
}



//*********************************************************************
//                      cmdBrew
//*********************************************************************

int cmdBrew(Player* player, cmd* cmnd) {
    if(!player->knowsSkill("alchemy")) {
        player->print("You have no idea how to brew potions!\n");
        return(0);
    }

    // Keep track of the player's alchemy skill level
    int skillLevel = (int)(player->getSkillGained("alchemy"));

    // Handle recipes later, for now a herb container is the target
    if(cmnd->num < 2) {
        player->print("Well, what would you like to brew?\n");
        return(0);
    }

    Object* mortar=0;   // Our Mortar and Pestle

    mortar = dynamic_cast<Object*>(player->findTarget(FIND_OBJ_INVENTORY | FIND_OBJ_EQUIPMENT | FIND_OBJ_ROOM, 0, cmnd->str[1], cmnd->val[1]));

    if(!mortar) {
        player->print("What would you like to brew the contents of?.\n");
        return(0);
    }


    if(mortar->getType() != ObjectType::CONTAINER || mortar->getSubType() != "mortar") {
        player->print("That isn't a mortar and pestle!\n");
        return(0);
    }

    if(mortar->objects.empty()) {
        player->print("But it's empty, what do you want to brew?\n");
        return(0);
    }

    if(mortar->getShotsCur() < 1) {
        player->print("You don't have enough herbs in there to brew anything.\n");
        return(0);
    }
    else if(mortar->getShotsCur() < 2 && skillLevel < 300) {
        player->print("You need at least two herbs to brew something.");
        return(0);
    }

    // Effects on the final potion
    std::map<bstring, AlchemyEffect> effects;

    if(mortar->getShotsCur() >= 2) {
        HerbMap effectCount;

        // We want to look at the first 4 herbs in the mortar and get a list of effects and how many occurrences of that
        // effect there are.  For any effect with 2 or more occurrences, it'll get added to the final potion
        // NOTE: You can only use as many effects as are visible for your alchemy level

        // If we have no effects with 2 or more occurrences, we have a failed attempt to make a potion.
        int numHerbs = 0;
        for(Object *herb : mortar->objects) {

            for(auto& p : herb->alchemyEffects) {

                bstring effect = p.second.getEffect();
                effectCount[effect].push_back(herb);

                if(effects.find(effect) == effects.end()) {
                    effects[effect] = p.second;
                } else {
                    // The effect is based on the minimum strength in the herbs
                    effects[effect].combineWith(p.second);

                }

            }
            if(++numHerbs == 4)
                break;
        } // end while



        for( HerbMap::value_type effectPair : effectCount) {
            if(effectPair.second.size() > 1) {
                player->printColor("Using effect: ^Y%s^x.\n", effectPair.first.c_str());
                for(Object* herb : effectPair.second) {
                    player->learnAlchemyEffect(herb, effectPair.first);
                }
            }
            else {
                effects.erase(effectPair.first);
            }
        } // end foreach
    }  // end if

    // We'll be using just one herb, so it'll be the first effect
    // To get here, we have to have 100 skill
    else if(mortar->getShotsCur() == 1) {
        Object* herb = *mortar->objects.begin();

        AlchemyEffect &ae = herb->alchemyEffects[1];
        effects[ae.getEffect()] = ae;
        player->printColor("Brewing a single effect potion: ^Y%s^x\n", ae.getEffect().c_str());
        player->learnAlchemyEffect(herb, ae.getEffect());
    }


    // If there's any positive effects then it's not a poison, default to poison
    bool isPotion = false;



    Object* potion = Object::getNewPotion();

    double alchemySkillModifier = player->getSkillGained("alchemy");

    int i = 1;
    // Copy the alchemy effects to the potion
    for(std::pair<bstring, AlchemyEffect> aep : effects) {
        AlchemyEffect eff = aep.second;
        long duration = 10;

        const AlchemyInfo* alc = gConfig->getAlchemyInfo(eff.getEffect());
        if(alc) {
            // Adjust things based on the alchemy info
            player->print("Found an alchemy Info!\n");
            duration = alc->getBaseDuration();
            duration = (long)((eff.getQuality() / 100.0) * duration);
            if(alc->isPositive())
                isPotion = true;
        }

        eff.setDuration(duration);
        player->print("Effect: %s Duration: %d\n", eff.getEffect().c_str(), eff.getDuration());
        potion->addAlchemyEffect(i++, eff);
    }

    potion->nameAlchemyPotion(isPotion);

    player->print("You have created %s!\n", potion->getCName());
    potion->setDroppedBy(player, "Craft:Alchemy");
    player->addObj(potion);
    return(0);
}

void Object::nameAlchemyPotion(bool potion) {
    std::ostringstream prefix;
    std::ostringstream suffix;

    if(potion)
        suffix << "potion of";
    else
        suffix << "poison of";
    bool valid = false;

    for(AlchemyEffectMap::value_type p : alchemyEffects) {
        const AlchemyInfo* alc = gConfig->getAlchemyInfo(p.second.getEffect());

        if(alc) {
            if(alc->potionNameHasPrefix()) {
                prefix << alc->getPotionPrefix() << " ";
            } else {
                suffix << " " << alc->getPotionDisplayName();
            }
        } else {
            suffix << " " << p.second.getEffect();
        }
        valid = true;
    }

    if(valid)
        setName(prefix.str() + suffix.str());
    else
        setName("murky potion");

}

//*********************************************************************
//                      addAlchemyEffect
//*********************************************************************

bool Object::addAlchemyEffect(int num, const AlchemyEffect &ae) {
    if(num < 0)
        return false;
    alchemyEffects[num] = ae;

    return true;
}

//*********************************************************************
//                      isAlchemyPotion
//*********************************************************************

bool Object::isAlchemyPotion() {
    return(type == ObjectType::POTION && alchemyEffects.size() > 0);
}

bool AlchemyInfo::potionNameHasPrefix() const {
    return(!potionPrefix.empty());
}

//*********************************************************************
//                      consumeAlchemyPotion
//*********************************************************************

// Return: Was it consumed?
bool Object::consumeAlchemyPotion(Creature* consumer) {
    if(!isAlchemyPotion() || !consumer)
        return false;
    // TODO: Verify we're not in a no potion room

    bool consumed = false;

    for(std::pair<int, AlchemyEffect> ae : alchemyEffects) {
        AlchemyEffect &eff = ae.second;
        const AlchemyInfo* alc = gConfig->getAlchemyInfo(eff.getEffect());
        if(!alc || alc->getAction() == "effect") {
            // If one of the effects takes hold, the potion was consumed
            if(eff.apply(consumer))
                consumed = true;
        }
        else if(alc->getAction()== "python") {
            if(gServer->runPython(alc->getPythonScript(), "", consumer, this))
                consumed = true;
        }
        else
            consumer->print("Unknown action: %s\n", alc->getAction().c_str());
    }

    return(consumed);
}

//*********************************************************************
//                      apply
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
        return(target->addEffect(effect, duration, strength) != nullptr);

    return(false);
}


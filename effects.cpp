/*
 * effects.cpp
 *   Effects
 *   ____            _
 *  |  _ \ ___  __ _| |_ __ ___  ___
 *  | |_) / _ \/ _` | | '_ ` _ \/ __|
 *  |  _ <  __/ (_| | | | | | | \__ \
 *  |_| \_\___|\__,_|_|_| |_| |_|___/
 *
 * Permission to use, modify and distribute is granted via the
 *  GNU Affero General Public License v3 or later
 *
 *  Copyright (C) 2007-2016 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

// C++ includes
#include <iomanip>
#include <locale>

#include "commands.h"
#include "creatures.h"
#include "config.h"
#include "effects.h"
#include "mud.h"
#include "raceData.h"
#include "rooms.h"
#include "server.h"
#include "socket.h"
#include "pythonHandler.h"
#include "xml.h"

//*********************************************************************
//                      getDisplayName
//*********************************************************************

bstring EffectInfo::getDisplayName() const {
    bstring displayName = myEffect->getDisplay();
    if(myEffect->getName() != "drunkenness")
        return(displayName);

    // drunkenness has a different name based on strength
    AlcoholState state = getAlcoholState(this);
    if(state == ALCOHOL_SOBER)
        displayName += "Sober";  // this state should never happen
    else if(state == ALCOHOL_TIPSY)
        displayName += "Tipsy";
    else if(state == ALCOHOL_DRUNK)
        displayName += "Drunk";
    else if(state == ALCOHOL_INEBRIATED)
        displayName += "Inebriated";

    displayName += "^x";
    return(displayName);
}

//*********************************************************************
//                      getAlcoholState
//*********************************************************************

AlcoholState getAlcoholState(const EffectInfo* effect) {
    if(!effect || effect->getStrength() < 1)
        return(ALCOHOL_SOBER);
    if(effect->getStrength() < 20)
        return(ALCOHOL_TIPSY);
    if(effect->getStrength() < 66)
        return(ALCOHOL_DRUNK);
    return(ALCOHOL_INEBRIATED);
}

//*********************************************************************
//                      clearEffects
//*********************************************************************

void Config::clearEffects() {
    for(std::pair<bstring, Effect*> ep : effects) {
        delete ep.second;
    }
    effects.clear();
}

//*********************************************************************
//                      getEffect
//*********************************************************************

Effect* Config::getEffect(bstring eName) {
    std::map<bstring, Effect*>::const_iterator eIt;
    if( (eIt = effects.find(eName)) == effects.end())
        return(nullptr);
    else
        return((*eIt).second);
}

//*********************************************************************
//                      effectExists
//*********************************************************************

bool Config::effectExists(bstring eName) {
    return(effects.find(eName) != effects.end());
}

//*********************************************************************
//                      dmEffectList
//*********************************************************************

int dmEffectList(Player* player, cmd* cmnd) {
    Effect* effect=0;
    bstring command = getFullstrText(cmnd->fullstr, 1);

    bool all = (command == "all");
    int id = command.toInt();

    player->printColor("^YEffects\n");
    player->printColor("Type ^y*effects all^x to see all effects or ^y*effects [num]^x to see a specific effect.\n");

    int i = 0;
    for(std::pair<bstring, Effect*> sp : gConfig->effects) {
        effect = sp.second;
        i++;

        if(id != 0 && i != id)
            continue;

        player->printColor("%d)\tName: ^W%-20s^x   Use Str: %s^x    Display: %s\n", i,
            effect->getName().c_str(), effect->usesStrength() ? "^gY" : "^rN",
            effect->getDisplay().c_str());

        if(!all && i != id)
            continue;

        player->printColor("\tOpposite Effect: %s\n", effect->getOppositeEffect().c_str());
        player->printColor("\tSelfAddStr: %s^x\n", effect->getSelfAddStr().c_str());
        player->printColor("\tRoomAddStr: %s^x\n", effect->getRoomAddStr().c_str());
        player->printColor("\tSelfDelStr: %s^x\n", effect->getSelfDelStr().c_str());
        player->printColor("\tRoomDelStr: %s^x\n", effect->getRoomDelStr().c_str());
        player->printColor("\tPulsed: %s^x\n", effect->isPulsed() ? "^GPulsed" : "Non-Pulsed");
        if(effect->isPulsed())
            *player << ColorOn << "\tPulse Delay: " << effect->getPulseDelay() << "^x\n" << ColorOff;
        player->printColor("\tSpell: %s^x\n", effect->isSpell() ? "^GYes" : "^RNo");
        player->printColor("\tType: %s^x\n", effect->getType().c_str());

        if(!effect->getApplyScript().empty())
            player->printColor("\tApply Script: %s\n", effect->getApplyScript().c_str());
        if(!effect->getPreApplyScript().empty())
            player->printColor("\tPre-Apply Script: %s\n", effect->getPreApplyScript().c_str());
        if(!effect->getPostApplyScript().empty())
            player->printColor("\tPost-Apply Script: %s\n", effect->getPostApplyScript().c_str());
        if(!effect->getComputeScript().empty())
            player->printColor("\tCompute Script: %s\n", effect->getComputeScript().c_str());
        if(!effect->getPulseScript().empty())
            player->printColor("\tPulse Script: %s\n", effect->getPulseScript().c_str());

        if(!effect->getUnApplyScript().empty())
            player->printColor("\tUnApplyScript: %s\n", effect->getUnApplyScript().c_str());
    }

    return(0);
}

//*********************************************************************
//                      willOverWrite
//*********************************************************************
// Will the given effect overwrite the existing effect?

bool EffectInfo::willOverWrite(EffectInfo* existingEffect) const {
    // No effect, it'll overwrite :-P
    if(!existingEffect)
        return(true);

    // If the existing effect is due to an applier (currently only worn objects),
    // there's no way to override that effect.
    if(existingEffect->getApplier())
        return(false);

    // Perm effects take presedence
    if(duration == -1 && existingEffect->duration != -1)
        return(true);

    if(strength < existingEffect->strength)
        return(false);

    // Don't overwrite perm effects with non perm
    if(existingEffect->duration == -1 && duration != -1)
        return(false);

    return(true);
}

//*********************************************************************
//                      operator<<
//*********************************************************************

std::ostream& operator<<(std::ostream& out, const EffectInfo& eff) {
    out.setf(std::ios::right, std::ios::adjustfield);
    bstring display = eff.getDisplayName();

    short count=0;
    int len = display.getLength();
    for(int i=0; i<len; i++) {
        if(display.getAt(i) == '^') {
            count += 2;
            i++;
        }
    }
    out << std::setw(38+count) << display << " - ";

    if(eff.duration == -1)
        out << "Permanent!";
    else
        out << timeStr(eff.duration);

    return(out);
}

//*********************************************************************
//                      pulse
//*********************************************************************
// True on a sucessful pulse
// False if it's time to wear off

bool EffectInfo::pulse(time_t t) {
    ASSERTLOG(myParent);

    bool wearOff = updateLastMod(t);
    if(wearOff)
        return(false);

    if(myEffect->isPulsed()) {
        if(timeForPulse(t))
            return(runScript(myEffect->getPulseScript()));
     }
    return(true);
}

//*********************************************************************
//                      updateLastMod
//*********************************************************************
// True if it's time for this effect to wear off

bool EffectInfo::updateLastMod(time_t t) {
    time_t diff = t - lastMod;
    lastMod = t;
    diff = MIN(MAX(0, duration), diff);
    duration -= diff;

    if(myApplier) {
        // for object appliers, keep the duration on the object in-sync with the effect duration
        Object* object = myApplier->getAsObject();
        if(object)
            object->setEffectDuration(duration);
    }

    return(duration == 0);
}

//*********************************************************************
//                      timeForPulse
//*********************************************************************

bool EffectInfo::timeForPulse(time_t t) {
    time_t diff = t - lastPulse;
    if(diff < myEffect->getPulseDelay())
        return(false);

    lastPulse = t;
    return(true);
}
//*********************************************************************
//                      remove
//*********************************************************************

bool EffectInfo::remove(bool show) {
    bool success = false;
    ASSERTLOG(myParent);

    if(myEffect) {
        success = true;
        if(show) {
            Creature* cParent = myParent->getAsCreature();
            if(cParent) {
                if(!myEffect->getSelfDelStr().empty())
                    cParent->printColor("%s\n", myEffect->getSelfDelStr().c_str());
                if(!myEffect->getRoomDelStr().empty() && cParent->getRoomParent())
                    cParent->getRoomParent()->effectEcho(myEffect->getRoomDelStr(), cParent);
            }
            Exit* xParent = myParent->getAsExit();
            if(xParent) {
                if(!myEffect->getRoomDelStr().empty()) {
                    if(xParent->getRoom())
                        xParent->getRoom()->effectEcho(myEffect->getRoomDelStr(), xParent);
                    else
                        std::cout << "Exit with no parent room!!!\n";
                }
            }
            BaseRoom* rParent = myParent->getAsRoom();
            if(rParent) {
                if(!myEffect->getRoomDelStr().empty())
                    rParent->effectEcho(myEffect->getRoomDelStr());
            }
        }
        success &= runScript(myEffect->getUnApplyScript());
    }


    if(myApplier) {
        // If object appliers are being worn when we remove the effect,
        // the object will be broken and unequipped
        Object* object = myApplier->getAsObject();
        if(object && object->flagIsSet(O_WORN)) {
            object->setShotsCur(0);
            myParent->getAsPlayer()->breakObject(object, object->getWearflag());
        }
    }

    return(success);
}

//*********************************************************************
//                      compute
//*********************************************************************

bool EffectInfo::compute(MudObject* applier) {
    ASSERTLOG(myParent);
    if(!myEffect)
        return(false);
    myApplier = applier;
    bool retVal = runScript(myEffect->getComputeScript(), applier);
    // Incase computing results in a non permanent effect that has a negative duration!
    if(this->duration < -1) {
        duration = 60;
    }
    return(retVal);
}

//*********************************************************************
//                      add
//*********************************************************************

bool EffectInfo::add() {
    ASSERTLOG(myParent);
    if(!myEffect)
        return(false);

    Creature* cParent = myParent->getAsCreature();
    if(cParent) {

        if(!myEffect->getSelfAddStr().empty())
            cParent->printColor("%s\n", cParent->doReplace(myEffect->getSelfAddStr().c_str(), cParent, myApplier).c_str());

        // TODO: replace *ACTOR* etc
        if(!myEffect->getRoomAddStr().empty() && cParent->getRoomParent())
            cParent->getRoomParent()->effectEcho(myEffect->getRoomAddStr(), cParent, myApplier);
    }
    Exit* xParent = myParent->getAsExit();
    if(xParent) {
        if(!myEffect->getRoomAddStr().empty()) {
            if(xParent->getRoom())
                xParent->getRoom()->effectEcho(myEffect->getRoomAddStr(), xParent);
            else
                std::cout << "Exit with no parent room!!!\n";
        }
    }
    BaseRoom* rParent = myParent->getAsRoom();
    if(rParent) {
        if(!myEffect->getRoomAddStr().empty())
            rParent->effectEcho(myEffect->getRoomAddStr());
    }

    return(true);
}

//*********************************************************************
//                      apply
//*********************************************************************

bool EffectInfo::apply() {
    ASSERTLOG(myParent);
    if(!myEffect)
        return(false);
    return(runScript(myEffect->getApplyScript()));
}

//*********************************************************************
//                      preApply
//*********************************************************************

bool EffectInfo::preApply() {
    ASSERTLOG(myParent);
    if(!myEffect)
        return(false);
    return(runScript(myEffect->getPreApplyScript()));
}

//*********************************************************************
//                      postApply
//*********************************************************************

bool EffectInfo::postApply(bool keepApplier) {
    bool success = false;
    ASSERTLOG(myParent);
    if(myEffect)
        success = runScript(myEffect->getPostApplyScript());

    // If you're passing in keepApplier = true, be sure you know what you're doing!
    // The effect will keep a pointer to the applier, and if the applier is removed
    // from memory (the monster is killed, the object is pawned), you'll get an
    // invalid pointer that might crash the game. Currently only equipped object
    // appliers are supported for passing in keepApplier = true.
    if(!keepApplier)
        myApplier = 0;

    return(success);
}

// End - EffectInfo functions


//*********************************************************************
//                      addEffect
//*********************************************************************


EffectInfo* MudObject::addEffect(const bstring& effect, long duration, int strength, MudObject* applier, bool show, const Creature* owner, bool keepApplier) {
    return(effects.addEffect(effect, duration, strength, applier, show, this, owner, keepApplier));
}

EffectInfo* Effects::addEffect(const bstring& effect, long duration, int strength, MudObject* applier, bool show, MudObject* pParent, const Creature* owner, bool keepApplier) {
    if(!gConfig->getEffect(effect))
        return(nullptr);
    EffectInfo* newEffect = new EffectInfo(effect, time(0), duration, strength, pParent, owner);

    if(!newEffect->compute(applier)) {
        delete newEffect;
        return(nullptr);
    }


    if(strength != -2)
        newEffect->setStrength(strength);
    if(duration != -2)
        newEffect->setDuration(duration);

    return(addEffect(newEffect, show, 0, keepApplier));
}

EffectInfo* MudObject::addEffect(EffectInfo* newEffect, bool show, bool keepApplier) {
    return(effects.addEffect(newEffect, show, this, keepApplier));
}

EffectInfo* Effects::addEffect(EffectInfo* newEffect, bool show, MudObject* pParent, bool keepApplier) {
    if(pParent)
        newEffect->setParent(pParent);

    EffectInfo* oldEffect = getExactEffect(newEffect->getName());
    bool success = true;

    if(oldEffect && !newEffect->willOverWrite(oldEffect)) {
        // The new effect won't overwrite, so don't add it
        if(pParent->getAsPlayer() && show)
            pParent->getAsPlayer()->print("The effect didn't take hold.\n");
        delete newEffect;
        return(nullptr);
    }

    // pre-apply gets called BEFORE the replaced effect gets removed
    newEffect->preApply();

    // If no existing effect, or this one will overwrite remove the old one, and add the new one
    removeEffect(newEffect->getName(), false, true);        // Don't show the removal

    // Only show if we're not overwriting an effect
    if(!oldEffect && show)
        newEffect->add();
    success &= newEffect->apply();
    effectList.push_back(newEffect);
    if(newEffect->getParent()->getAsRoom())
        newEffect->getParent()->getAsRoom()->addEffectsIndex();
    else if(newEffect->getParent()->getAsExit() && newEffect->getParent()->getAsExit()->getRoom())
        newEffect->getParent()->getAsExit()->getRoom()->addEffectsIndex();

    // post-apply gets run after everything is done
    newEffect->postApply(keepApplier);

    return(newEffect);
}

//*********************************************************************
//                      addPermEffect
//*********************************************************************

EffectInfo* MudObject::addPermEffect(const bstring& effect, int strength, bool show) {
    return(effects.addEffect(effect, -1, strength, nullptr, show, this));
}

//*********************************************************************
//                      removeEffect
//*********************************************************************
// Remove the effect (Won't remove permanent effects if remPerm is false)

bool MudObject::removeEffect(const bstring& effect, bool show, bool remPerm, MudObject* fromApplier) {
    return(effects.removeEffect(effect, show, remPerm, fromApplier));
}

bool Effects::removeEffect(const bstring& effect, bool show, bool remPerm, MudObject* fromApplier) {
    EffectInfo* toDel = getExactEffect(effect);
    if( toDel &&
        (toDel->getDuration() != -1 || (toDel->getDuration() == -1 && remPerm)) &&
        (!fromApplier || fromApplier == toDel->getApplier())
    )
        return(removeEffect(toDel, show));
    return(false);
}

bool MudObject::removeEffect(EffectInfo* toDel, bool show) {
    return(effects.removeEffect(toDel, show));
}

bool Effects::removeEffect(EffectInfo* toDel, bool show) {
    if(!toDel)
        return(false);

    effectList.remove(toDel);
    toDel->remove(show);
    delete toDel;
    return(true);
}

//*********************************************************************
//                      removeOwner
//*********************************************************************
// on suicide, we remove the owner of the effect

void Effects::removeOwner(const Creature* owner) {
    EffectList::iterator it;

    for(it = effectList.begin() ; it != effectList.end() ; it++) {
        if((*it)->isOwner(owner))
            (*it)->setOwner(0);
    }
}

//*********************************************************************
//                      objectCanBestowEffect
//*********************************************************************

bool Effect::objectCanBestowEffect(const bstring& effect) {
    return( effect != "" &&
            effect != "vampirism" &&
            effect != "porphyria" &&
            effect != "lycanthropy"
    );
}

//*********************************************************************
//                      isEffected
//*********************************************************************

bool MudObject::isEffected(const bstring& effect, bool exactMatch) const {
    return(effects.isEffected(effect, exactMatch));
}

bool MudObject::isEffected(EffectInfo* effect) const {
    return(effects.isEffected(effect));
}

// We are effected if we have an effect with this name, or an effect with a base effect
// of this name

bool Effects::isEffected(const bstring& effect, bool exactMatch) const {
    //EffectList list;
    for(EffectInfo* eff : effectList) {
        if(eff->getName() == effect || (!exactMatch && eff->hasBaseEffect(effect)))
            return(true);
    }

    return(false);
}

bool Effects::isEffected(EffectInfo* effect) const {
    for(EffectInfo* eff : effectList) {
        if(eff->getName() == effect->getName() || eff->hasBaseEffect(effect->getName()) || effect->hasBaseEffect(eff->getName()))
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      getBaseEffects
//*********************************************************************

const std::list<bstring>& Effect::getBaseEffects() {
    return(baseEffects);
}

// Effect - Avian Aria, Base Effects = fly, levitate
// eff = fly

// Effect - Avian Aria, Base Effects = fly, levitate
// Effect Some Jackass's Flying Song.  Base effect = fly, levitate

//*********************************************************************
//                      hasPermEffect
//*********************************************************************

bool MudObject::hasPermEffect(const bstring& effect) const {
    EffectInfo* toCheck = effects.getEffect(effect);
    return(toCheck && toCheck->getDuration() == -1);
}

//*********************************************************************
//                      getEffect
//*********************************************************************

EffectInfo* MudObject::getEffect(const bstring& effect) const {
    return(effects.getEffect(effect));
}
// Returns the effect if we find one that the name matches or has
// the base effect mentioned

EffectInfo* Effects::getEffect(const bstring& effect) const {
    EffectList::const_iterator eIt;
    EffectInfo* toReturn = nullptr;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        if((*eIt) && ((*eIt)->getName() == effect || (*eIt)->hasBaseEffect(effect))) {
            if(!toReturn)
                toReturn = (*eIt);
            else {
                // If we have something to return, compare it, we'll return the
                // effect with the highest strength
                if((*eIt)->getStrength() > toReturn->getStrength())
                    toReturn = (*eIt);
            }
        }
    }
    return(toReturn);
}

//*********************************************************************
//                      getExactEffect
//*********************************************************************

EffectInfo* MudObject::getExactEffect(const bstring& effect) const {
    return(effects.getExactEffect(effect));
}

// Returns the effect with an exact name match
EffectInfo* Effects::getExactEffect(const bstring& effect) const {
    EffectList::const_iterator eIt;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        if((*eIt) && (*eIt)->getName() == effect)
            return((*eIt));
    }
    return(nullptr);
}

//*********************************************************************
//                      pulseEffects
//*********************************************************************
// Pulse all effects on this creature
// return false if creature died because of this

bool Creature::pulseEffects(time_t t) {
    bool pulsed = true;
    bool poison = false;
    EffectInfo* effect=0;
    EffectList::iterator eIt;
    deathtype = DT_NONE;
    for(eIt = effects.effectList.begin() ; eIt != effects.effectList.end() ;) {
        effect = (*eIt);
        // Pulse!

        ASSERTLOG(effect->getParent() == this);
        pulsed = effect->pulse(t);

        // If pulse returns false, purge this effect
        if(!pulsed) {
            effect->remove();
            if(poison || effect->isPoison())
                poison = true;
            delete effect;
            eIt = effects.effectList.erase(eIt);
        } else
            eIt++;
    }

    // pulse effects might kill them
    if(deathtype != DT_NONE && hp.getCur() < 1) {
        if(isPlayer())
            getAsPlayer()->die(deathtype);
        else
            getAsMonster()->mobDeath();
        return(false);
    }

    // if they're not poisoned anymore, clear poison
    if(poison && !isPoisoned())
        curePoison();

    return(true);
}

//*********************************************************************
//                      pulseEffects
//*********************************************************************

bool BaseRoom::pulseEffects(time_t t) {
    effects.pulse(t, this);

    for(Exit* exit : exits) {
        exit->pulseEffects(t);
    }
    return(true);
}

//*********************************************************************
//                      pulseEffects
//*********************************************************************

bool Exit::pulseEffects(time_t t) {
    effects.pulse(t, this);
    return(true);
}

//*********************************************************************
//                      pulse
//*********************************************************************
// generic pulse function that can be used on rooms and exits (because
// they can't die like players can)

void Effects::pulse(time_t t, MudObject* pParent) {
    EffectList::iterator it;
    EffectInfo* effect=0;
    bool pulsed=false;

    for(it = effectList.begin() ; it != effectList.end() ;) {
        effect = (*it);
        // Pulse!

        ASSERTLOG(effect->getParent() == pParent);
        pulsed = effect->pulse(t);

        // If pulse returns false, purge this effect
        if(!pulsed) {
            effect->remove();
            delete effect;
            it = effectList.erase(it);
        } else
            it++;
    }
}

//*********************************************************************
//                      removeAll
//*********************************************************************

void Effects::removeAll() {
    EffectInfo* effect=0;
    EffectList::iterator eIt;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        effect = (*eIt);
        delete effect;
        (*eIt) = nullptr;
    }
    effectList.clear();
}

//*********************************************************************
//                      copy
//*********************************************************************

void Effects::copy(const Effects* source, MudObject* pParent) {
    EffectInfo* effect;
    EffectList::const_iterator eIt;
    for(eIt = source->effectList.begin() ; eIt != source->effectList.end() ; eIt++) {
        effect = new EffectInfo();
        (*effect) = *(*eIt);
        effect->setParent(pParent);
        effectList.push_back(effect);
    }
}

//*********************************************************************
//                      cmdEffects
//*********************************************************************

int cmdEffects(Creature* creature, cmd* cmnd) {
    Creature* target = creature;
    int num=0;

    if(creature->isCt()) {
        if(cmnd->num > 1) {
            target = creature->getParent()->findCreature(creature, cmnd);
            cmnd->str[1][0] = up(cmnd->str[1][0]);

            if(!target) {
                target = gServer->findPlayer(cmnd->str[1]);

                if(!target || !creature->canSee(target)) {
                    creature->print("Target not found.\n");
                    return(0);
                }
            }
        }
    }

    num = target->effects.effectList.size();
    creature->print("Current Effects for %s:\n", target->getCName());
    creature->printColor("%s", target->effects.getEffectsString(creature).c_str());
    creature->print("\n%d effect%s found.\n", num, num != 1 ? "s" : "");
    return(0);
}

//*********************************************************************
//                      getEffectsList
//*********************************************************************
// Return a list of effects

bstring Effects::getEffectsList() const {
    std::ostringstream effStr;

    effStr << "Effects: ";

    int num = 0;
    const EffectInfo* effect;
    EffectList::const_iterator eIt;
    for(eIt = effectList.begin() ; eIt != effectList.end() ; eIt++) {
        effect = (*eIt);
        if(num != 0)
            effStr << ", ";
        effStr << effect->getName();
        num++;
    }

    if(num == 0)
        effStr << "None";
    effStr << ".\n";

    bstring toPrint = effStr.str();

    return(toPrint);
}

//*********************************************************************
//                      getEffectsString
//*********************************************************************
// Used to print out what effects a creature is under

bstring Effects::getEffectsString(const Creature* viewer) {
    const Object* object=0;
    std::ostringstream effStr;
    long t = time(0);

    for(EffectInfo* effect : effectList) {
        effect->updateLastMod(t);
        effStr << *effect;
        if(viewer->isStaff()) {
//          if(!effect->getBaseEffect().empty())
//              effStr << " Base(" << effect->getBaseEffect() << ")";
            effStr << "  ^WStrength:^x " << effect->getStrength();
            if(effect->getExtra()) {
                effStr << "  ^WExtra:^x " << effect->getExtra();

                // extra information
                if(effect->getName() == "illusion") {
                    const RaceData* race = gConfig->getRace(effect->getExtra());
                    if(race)
                        effStr << " (" << race->getName() << ")";
                } else if(  (effect->getDuration() == -1) && (
                    effect->getName() == "wall-of-force" ||
                    effect->getName() == "wall-of-fire" ||
                    effect->getName() == "wall-of-thorns"
                ) ) {
                    // permanent walls are only down for a little bit
                    effStr << " (# pulses until reinstantiate)";
                }
            }
            if(effect->getApplier()) {
                object = effect->getApplier()->getAsConstObject();
                if(object)
                    effStr << "  ^WApplier:^x " << object->getName() << "^x";
            }
        }
        effStr << "\n";
    }

    return(effStr.str());
}

//*********************************************************************
//                      convertOldEffects
//*********************************************************************
// This function will convert flag/lt combos into effects

void Creature::convertOldEffects() {
    Player* pPlayer = getAsPlayer();
    Monster* mMonster = getAsMonster();
    if(version < "2.40") {
        if(mMonster) {
//          mMonster->convertToEffect("stoneskin", OLD_M_STONESKIN, -1);
//          mMonster->convertToEffect("invisibility", OLD_M_INVISIBLE, OLD_LT_INVISIBILITY);
        } else if(pPlayer) {

        }
    }
}

//*********************************************************************
//                      convertToEffect
//*********************************************************************
// Convert the given effect from a flag/lt to an effect

bool Creature::convertToEffect(const bstring& effect, int flag, int lt) {
    if(!flagIsSet(flag))
        return(false);

    clearFlag(flag);


    long duration = 0;
    if(lt != -1 && lasttime[lt].interval != 0)
        duration = lasttime[lt].interval;
    else
        duration = -1;

    EffectInfo* newEffect = new EffectInfo(effect, time(0), duration, 1, this);

//  if(lt != -1 && (effect == "armor" || effect == "stoneskin")) {
//      newEffect->setStrength(lasttime[lt].misc);
//  }

    // Assuming that they're already properly under the effect, so just add it to the list
    // and don't actually add it or compute it.
    // IE: Strength buff -- they already have +str, so don't give them more str!!
    effects.effectList.push_back(newEffect);
    return(true);
}

//*********************************************************************
//                      removeOppositeEffect
//*********************************************************************

bool MudObject::removeOppositeEffect(const EffectInfo *effect) {
    return(effects.removeOppositeEffect(effect));
}

bool Effects::removeOppositeEffect(const EffectInfo *effect) {
    Effect* parentEffect = effect->getEffect();

    if(!parentEffect)
        return(false);

    if(parentEffect->getOppositeEffect().empty())
        return(false);

    return(removeEffect(parentEffect->getOppositeEffect(), true, true));
}

//*********************************************************************
//                      exitEffectDamage
//*********************************************************************
// Wrapper to actually do the damage to the target
// Return: true if they were killed

bool exitEffectDamage(const EffectInfo *effect, Creature* target, Creature* owner, Realm realm, DeathType dt, const char* crtStr, const char* deathStr, const char* roomStr, const char* killerStr) {
    Damage damage;

    if(!effect || effect->getExtra() || effect->isOwner(owner))
        return(false);

    Player* killer=0;
    bool online = true;

    if(effect->getOwner() != "") {
        killer = gServer->findPlayer(effect->getOwner());
        if(!killer) {
            if(loadPlayer(effect->getOwner().c_str(), &killer))
                online = false;
            else
                killer = 0;
        }
    }

    damage.set(mrand(effect->getStrength() / 2, effect->getStrength() * 3 / 2));
    target->modifyDamage(0, MAGICAL, damage, realm);

    if(killer && target->isMonster()) {
        target->getAsMonster()->addEnemy(killer);
        if(online)
            target->getAsMonster()->adjustThreat(killer, damage.get());
    }

    target->printColor(crtStr, target->customColorize("*CC:DAMAGE*").c_str(), damage.get());

    target->hp.decrease(damage.get());
    if(target->hp.getCur() < 1) {
        target->print(deathStr);
        broadcast(target->getSock(), target->getRoomParent(), roomStr, target, target);

        if(killer) {
            if(online)
                killer->print(killerStr, target);
            target->die(killer);
            killer->save(online);
            if(!online)
                free_crt(killer);
        } else {
            if(target->isPlayer())
                target->getAsPlayer()->die(dt);
            else
                target->getAsMonster()->mobDeath();
        }
        return(true);
    }
    if(killer && !online)
        free_crt(killer);
    return(false);
}

//*********************************************************************
//                      doEffectDamage
//*********************************************************************
// Return: true if they were killed

bool Exit::doEffectDamage(Creature* target) {
    Creature *owner = target;

    if(target->isPet())
        owner = target->getMaster();

    if( exitEffectDamage(
            getEffect("wall-of-fire"),
            target, owner, FIRE, BURNED,
            "The wall of fire burns you for %s%d^x damage.\n",
            "You are burned to death!\n",
            "%1M is engulfed by the wall of fire.\n%M burns to death!",
            "%M is engulfed by your wall of fire and is incinerated!\n"
    ) )
        return(true);

    if( exitEffectDamage(
            getEffect("wall-of-thorns"),
            target, owner, EARTH, THORNS,
            "The wall of thorns stabs you for %s%d^x damage.\n",
            "You are stabbed to death!\n",
            "%1M is engulfed by a wall of thorns.\n%M is stabbed to death!",
            "%M is engulfed by your wall of thorns and is stabbed to death!\n"
    ) )
        return(true);

    return(false);
}

//*********************************************************************
//                      doReplace
//*********************************************************************

bstring Creature::doReplace(bstring fmt, const MudObject* actor, const MudObject* applier) const {
    const Creature* cActor = actor->getAsConstCreature();
    const Exit* xActor = actor->getAsConstExit();
    const Creature* cApplier = applier->getAsConstCreature();

    if(cActor) {
        fmt.Replace("*ACTOR*", cActor->getCrtStr(this, CAP).c_str());
        fmt.Replace("*LOW-ACTOR*", cActor->getCrtStr(this).c_str());
        fmt.Replace("*A-HISHER*", cActor->hisHer());
        fmt.Replace("*A-UPHISHER*", cActor->upHisHer());
    } else if(xActor) {
        fmt.Replace("*ACTOR*", xActor->getCName());
        fmt.Replace("*LOW-ACTOR*", xActor->getCName());
    }

    if(cApplier) {
        // Applier Possessive
        if(cActor == cApplier) {
            fmt.Replace("*APPLIER-POS*", cApplier->upHisHer());
            fmt.Replace("*LOW-APPLIER-POS*", cApplier->hisHer());
            fmt.Replace("*APPLIER-SELF-POS*", "Your");
            fmt.Replace("*LOW-APPLIER-SELF-POS*", "your");
        } else {
            fmt.Replace("*APPLIER-POS*", bstring(cApplier->getCrtStr(this, CAP) + "'s").c_str());
            fmt.Replace("*LOW-APPLIER-POS*", bstring(cApplier->getCrtStr(this) + "'s").c_str());
            fmt.Replace("*APPLIER-SELF-POS*", bstring(cApplier->getCrtStr(this, CAP) + "'s").c_str());
            fmt.Replace("*LOW-APPLIER-SELF-POS*", bstring(cApplier->getCrtStr(this) + "'s").c_str());
        }

        fmt.Replace("*APPLIER*", cApplier->getCrtStr(this, CAP).c_str());
        fmt.Replace("*LOW-APPLIER*", cApplier->getCrtStr(this).c_str());
        fmt.Replace("*AP-HISHER*", cApplier->hisHer());
        fmt.Replace("*AP-UPHISHER*", cApplier->upHisHer());
    }
    return(fmt);
}

//*********************************************************************
//                      effectEcho
//*********************************************************************

void Container::effectEcho(bstring fmt, const MudObject* actor, const MudObject* applier, Socket* ignore) {
    Socket* ignore2 = nullptr;
    if(actor->getAsConstCreature())
        ignore2 = actor->getAsConstCreature()->getSock();
    for(const Player* ply : players) {
        if(!ply || (ply->getSock() && (ply->getSock() == ignore || ply->getSock() == ignore2)) || ply->isUnconscious())
            continue;

        bstring toSend = ply->doReplace(fmt, actor, applier);
        ply->bPrint(toSend + "\n");
    }
}

//*********************************************************************
//                      getPulseDelay
//*********************************************************************

int Effect::getPulseDelay() const {
    return(pulseDelay);
}

//*********************************************************************
//                      runScript
//*********************************************************************

bool EffectInfo::runScript(const bstring& pyScript, MudObject* applier) {

    // Legacy: Default action is return true, so play along with that
    if(pyScript.empty())
        return(true);

    try {

        boost::python::object localNamespace( (boost::python::handle<>(PyDict_New())));

        boost::python::object effectModule( (boost::python::handle<>(PyImport_ImportModule("effectLib"))) );

        localNamespace["effectLib"] = effectModule;

        localNamespace["effect"] = boost::python::ptr(this);

        // Default retVal is true
        localNamespace["retVal"] = true;
        addMudObjectToDictionary(localNamespace, "actor", myParent);
        addMudObjectToDictionary(localNamespace, "applier", applier);



        gServer->runPython(pyScript, localNamespace);

        bool retVal = boost::python::extract<bool>(localNamespace["retVal"]);
        //std::cout << "runScript returning: " << retVal << std::endl;
        return(retVal);
    }
    catch( boost::python::error_already_set) {
        gServer->handlePythonError();
    }

    return(false);
}


//*********************************************************************
//                      pulseCreatureEffects
//*********************************************************************
// lastUserUpdate is set in updateUsers

void Server::pulseCreatureEffects(long t) {
    Monster *monster=0;
    const Socket *sock=0;
    Player* player=0;
    std::list<Socket*>::const_iterator it;

    for(it = sockets.begin(); it != sockets.end() ; ) {
        sock = *it;
        it++;
        if(!sock->isConnected())
            continue;
        player = sock->getPlayer();
        if(player)
            player->pulseEffects(t);
    }

    MonsterList::iterator mIt = activeList.begin();
    while(mIt != activeList.end()) {
        // Increment the iterator in case this monster dies during the update and is removed from the active list
        monster = (*mIt++);
        monster->pulseEffects(t);
    }
}

//*********************************************************************
//                      pulseRoomEffects
//*********************************************************************

void Server::pulseRoomEffects(long t) {
    std::list<BaseRoom*>::iterator it;

    for(it = effectsIndex.begin() ; it != effectsIndex.end() ; ) {
        (*it)->pulseEffects(t);

        if(!(*it)->needsEffectsIndex())
            it = effectsIndex.erase(it);
        else
            it++;
    }

    lastRoomPulseUpdate = t;
}

//*********************************************************************
//                      showEffectsIndex
//*********************************************************************

void Server::showEffectsIndex(const Player* player) {
    std::list<BaseRoom*>::const_iterator it;
    int i=0;

    player->printColor("^YRoom Effects Index\n");

    for(it = effectsIndex.begin() ; it != effectsIndex.end() ; it++) {
        player->print("%s\n", (*it)->fullName().c_str());
        i++;
    }

    player->print("%d room%s in effects index.\n", i, i==1 ? "" : "s");
}

//*********************************************************************
//                      dmShowEffectsIndex
//*********************************************************************

int dmShowEffectsIndex(Player* player, cmd* cmnd) {
    gServer->showEffectsIndex(player);
    return(0);
}
//*********************************************************************
//                      addEffectsIndex
//*********************************************************************

void BaseRoom::addEffectsIndex() {
    if(!needsEffectsIndex())
        return;
    gServer->addEffectsIndex(this);
}

void Server::addEffectsIndex(BaseRoom* room) {
    // you can only be in the list once!
    std::list<BaseRoom*>::const_iterator it;
    for(it = effectsIndex.begin() ; it != effectsIndex.end() ; it++) {
        if((*it) == room)
            return;
    }

    effectsIndex.push_back(room);
}

//*********************************************************************
//                      needsEffectsIndex
//*********************************************************************

bool BaseRoom::needsEffectsIndex() const {
    // any room effects?
    if(effects.effectList.size())
        return(true);

    // any exit effects?
    for(Exit* exit : exits) {
        if(exit->effects.effectList.size())
            return(true);
    }

    return(false);
}

//*********************************************************************
//                      removeEffectsIndex
//*********************************************************************

bool BaseRoom::removeEffectsIndex() {
    if(needsEffectsIndex())
        return(false);
    gServer->removeEffectsIndex(this);
    return(true);
}

void Server::removeEffectsIndex(BaseRoom* room) {
    std::list<BaseRoom*>::iterator it;
    for(it = effectsIndex.begin() ; it != effectsIndex.end() ; it++) {
        if((*it) == room) {
            effectsIndex.erase(it);
            return;
        }
    }
}

//*********************************************************************
//                      removeEffectsOwner
//*********************************************************************
// on suicide, we remove the owner of the effect

void Server::removeEffectsOwner(const Creature* owner) {
    std::list<BaseRoom*>::iterator it;

    for(it = effectsIndex.begin() ; it != effectsIndex.end() ; it++) {
        (*it)->effects.removeOwner(owner);
        for(Exit* exit : (*it)->exits) {
            exit->effects.removeOwner(owner);
        }
    }
}


//*********************************************************************
//                      Effect
//*********************************************************************

Effect::Effect(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    pulsed = isSpellEffect = usesStr = false;
    pulseDelay = 5;

    while(curNode) {
             if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "BaseEffect")) baseEffects.push_back(xml::getBString(curNode));
        else if(NODE_NAME(curNode, "Display")) xml::copyToBString(display, curNode);
        else if(NODE_NAME(curNode, "OppositeEffect")) xml::copyToBString(oppositeEffect, curNode);
        else if(NODE_NAME(curNode, "SelfAddStr")) xml::copyToBString(selfAddStr, curNode);
        else if(NODE_NAME(curNode, "SelfDelStr")) xml::copyToBString(selfDelStr, curNode);
        else if(NODE_NAME(curNode, "RoomAddStr")) xml::copyToBString(roomAddStr, curNode);
        else if(NODE_NAME(curNode, "RoomDelStr")) xml::copyToBString(roomDelStr, curNode);
        else if(NODE_NAME(curNode, "Pulsed")) xml::copyToBool(pulsed, curNode);
        else if(NODE_NAME(curNode, "PulseDelay")) xml::copyToNum(pulseDelay, curNode);
        else if(NODE_NAME(curNode, "Type")) xml::copyToBString(type, curNode);
        else if(NODE_NAME(curNode, "ComputeScript")) xml::copyToBString(computeScript, curNode);
        else if(NODE_NAME(curNode, "ApplyScript")) xml::copyToBString(applyScript, curNode);
        else if(NODE_NAME(curNode, "PreApplyScript")) xml::copyToBString(preApplyScript, curNode);
        else if(NODE_NAME(curNode, "PostApplyScript")) xml::copyToBString(postApplyScript, curNode);
        else if(NODE_NAME(curNode, "UnApplyScript")) xml::copyToBString(unApplyScript, curNode);
        else if(NODE_NAME(curNode, "PulseScript")) xml::copyToBString(pulseScript, curNode);
        else if(NODE_NAME(curNode, "Spell")) xml::copyToBool(isSpellEffect, curNode);
        else if(NODE_NAME(curNode, "UsesStrength")) xml::copyToBool(usesStr, curNode);

        curNode = curNode->next;
    }
}

//*********************************************************************
//                      getPulseScript
//*********************************************************************

bstring Effect::getPulseScript() const {
    return(pulseScript);
}

//*********************************************************************
//                      getUnApplyScript
//*********************************************************************

bstring Effect::getUnApplyScript() const {
    return(unApplyScript);
}

//*********************************************************************
//                      getApplyScript
//*********************************************************************

bstring Effect::getApplyScript() const {
    return(applyScript);
}

//*********************************************************************
//                      getPreApplyScript
//*********************************************************************

bstring Effect::getPreApplyScript() const {
    return(preApplyScript);
}

//*********************************************************************
//                      getPostApplyScript
//*********************************************************************

bstring Effect::getPostApplyScript() const {
    return(postApplyScript);
}

//*********************************************************************
//                      getComputeScript
//*********************************************************************

bstring Effect::getComputeScript() const {
    return(computeScript);
}

//*********************************************************************
//                      getType
//*********************************************************************

bstring Effect::getType() const {
    return(type);
}

//*********************************************************************
//                      isPulsed
//*********************************************************************

bool Effect::isPulsed() const {
    return(pulsed);
}

//*********************************************************************
//                      isSpell
//*********************************************************************

bool Effect::isSpell() const {
    return(isSpellEffect);
}

//*********************************************************************
//                      isSpell
//*********************************************************************

bool Effect::usesStrength() const {
    return(usesStr);
}

//*********************************************************************
//                      getRoomDelStr
//*********************************************************************

bstring Effect::getRoomDelStr() const {
    return(roomDelStr);
}

//*********************************************************************
//                      getRoomAddStr
//*********************************************************************

bstring Effect::getRoomAddStr() const {
    return(roomAddStr);
}

//*********************************************************************
//                      getSelfDelStr
//*********************************************************************

bstring Effect::getSelfDelStr() const {
    return(selfDelStr);
}

//*********************************************************************
//                      getSelfAddStr
//*********************************************************************

bstring Effect::getSelfAddStr() const {
    return(selfAddStr);
}

//*********************************************************************
//                      getOppositeEffect
//*********************************************************************

bstring Effect::getOppositeEffect() const {
    return(oppositeEffect);
}

//*********************************************************************
//                      getDisplay
//*********************************************************************

bstring Effect::getDisplay() const {
    return(display);
}

//*********************************************************************
//                      hasBaseEffect
//*********************************************************************

bool EffectInfo::hasBaseEffect(const bstring& effect) const {
    return(myEffect->hasBaseEffect(effect));
}

//*********************************************************************
//                      hasBaseEffect
//*********************************************************************

bool Effect::hasBaseEffect(const bstring& effect) const {
    for(const bstring& be : baseEffects) {
        if(be == effect)
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      getName
//*********************************************************************

bstring Effect::getName() const {
    return(name);
}

//*********************************************************************
//                      EffectInfo
//*********************************************************************
// TODO: Add applier here

EffectInfo::EffectInfo(bstring pName, time_t pLastMod, long pDuration, int pStrength, MudObject* pParent, const Creature* owner):
        name(pName), lastMod(pLastMod), lastPulse(pLastMod), duration(pDuration), strength(pStrength), myParent(pParent)
{
    myEffect = gConfig->getEffect(pName);
    if(!myEffect)
        throw bstring("Can't find effect " + pName);
    setOwner(owner);
}

//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::EffectInfo()
{

}

//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::EffectInfo(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
            if(NODE_NAME(curNode, "Name")) xml::copyToBString(name, curNode);
        else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);
        else if(NODE_NAME(curNode, "Strength")) xml::copyToNum(strength, curNode);
        else if(NODE_NAME(curNode, "Extra")) xml::copyToNum(extra, curNode);
        else if(NODE_NAME(curNode, "PulseModifier")) xml::copyToNum(pulseModifier, curNode);

        curNode = curNode->next;
    }

    lastPulse = lastMod = time(0);
    myEffect = gConfig->getEffect(name);

    if(!myEffect) {
        throw bstring("Can't find effect listing " + name);
    }
}

//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::~EffectInfo() {
}

//*********************************************************************
//                      setParent
//*********************************************************************

void EffectInfo::setParent(MudObject* pParent) {
    myParent = pParent;
}

//*********************************************************************
//                      getEffect
//*********************************************************************

Effect* EffectInfo::getEffect() const {
    return(myEffect);
}

//*********************************************************************
//                      getName
//*********************************************************************

const bstring EffectInfo::getName() const {
    return(name);
}

//*********************************************************************
//                      getOwner
//*********************************************************************

const bstring EffectInfo::getOwner() const {
    return(pOwner);
}

//*********************************************************************
//                      isOwner
//*********************************************************************

bool EffectInfo::isOwner(const Creature* owner) const {
    // currently, only players can own effects
    return(owner && owner->isPlayer() && pOwner == owner->getName());
}

//*********************************************************************
//                      getLastMod
//*********************************************************************

time_t EffectInfo::getLastMod() const {
    return(lastMod);
}

//*********************************************************************
//                      getDuration
//*********************************************************************

long EffectInfo::getDuration() const {
    return(duration);
}

//*********************************************************************
//                      getStrength
//*********************************************************************

int EffectInfo::getStrength() const {
    return(strength);
}

//*********************************************************************
//                      getExtra
//*********************************************************************

int EffectInfo::getExtra() const {
    return(extra);
}

//*********************************************************************
//                      isPermanent
//*********************************************************************

bool EffectInfo::isPermanent() const {
    return(duration == -1);
}

//*********************************************************************
//                      getParent
//*********************************************************************

MudObject* EffectInfo::getParent() const {
    return(myParent);
}

//*********************************************************************
//                      getApplier
//*********************************************************************

MudObject* EffectInfo::getApplier() const {
    return(myApplier);
}

//*********************************************************************
//                      setOwner
//*********************************************************************

void EffectInfo::setOwner(const Creature* owner) {
    if(owner)
        pOwner = owner->getName();
    else
        pOwner = "";
}

//*********************************************************************
//                      setStrength
//*********************************************************************

void EffectInfo::setStrength(int pStrength) {
    strength = pStrength;
}

//*********************************************************************
//                      setExtra
//*********************************************************************

void EffectInfo::setExtra(int pExtra) {
    extra = pExtra;
}

//*********************************************************************
//                      setDuration
//*********************************************************************

void EffectInfo::setDuration(long pDuration) {
    duration = pDuration;
}

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
 *  Copyright (C) 2007-2021 Jason Mitchell, Randi Mitchell
 *     Contributions by Tim Callahan, Jonathan Hseu
 *  Based on Mordor (C) Brooke Paul, Brett J. Vickers, John P. Freeman
 *
 */

#include <ctime>                                    // for time, time_t
#include <iomanip>                                  // for operator<<, setw
#include <boost/algorithm/string/replace.hpp>

#include "cmd.hpp"                                  // for cmd
#include "commands.hpp"                             // for getFullstrText
#include "config.hpp"                               // for Config, gConfig
#include "container.hpp"                            // for Container, PlayerSet
#include "creatureStreams.hpp"                      // for Streamable, ColorOff
#include "creatures.hpp"                            // for Creature, Player
#include "damage.hpp"                               // for Damage
#include "effects.hpp"                              // for EffectInfo, Effect
#include "exits.hpp"                                // for Exit
#include "flags.hpp"                                // for O_WORN
#include "free_crt.hpp"                             // for free_crt
#include "global.hpp"                               // for CAP, DT_NONE, BURNED
#include "join.hpp"                                 // for join
#include "mudObject.hpp"                            // for MudObject
#include "objects.hpp"                              // for Object
#include "os.hpp"                                   // for ASSERTLOG
#include "proto.hpp"                                // for broadcast
#include "pythonHandler.hpp"                        // for addMudObjectToDic...
#include "raceData.hpp"                             // for RaceData
#include "random.hpp"                               // for Random
#include "realm.hpp"                                // for EARTH, FIRE, Realm
#include "rooms.hpp"                                // for BaseRoom, ExitList
#include "server.hpp"                               // for Server, gServer
#include "socket.hpp"                               // for Socket
#include "structs.hpp"                              // for ALCOHOL_DRUNK
#include "utils.hpp"                                // for MAX, MIN
#include "xml.hpp"                                  // for copyToString
#include "toNum.hpp"

//*********************************************************************
//                      getDisplayName
//*********************************************************************

std::string EffectInfo::getDisplayName() const {
    std::string displayName = myEffect->getDisplay();
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
//                      getEffect
//*********************************************************************

const Effect* Config::getEffect(const std::string &eName) {
    EffectMap::const_iterator eIt;
    if( (eIt = effects.find(eName)) == effects.end())
        return(nullptr);
    else
        return(&((*eIt).second));
}

//*********************************************************************
//                      effectExists
//*********************************************************************

bool Config::effectExists(const std::string &eName) {
    return(effects.find(eName) != effects.end());
}

//*********************************************************************
//                      dmEffectList
//*********************************************************************

int dmEffectList(Player* player, cmd* cmnd) {
    std::string command = getFullstrText(cmnd->fullstr, 1);

    bool all = (command == "all");
    int id = toNum<int>(command);

    player->printPaged("^YEffects\n");
    player->printPaged("Type ^y*effects all^x to see all effects or ^y*effects [num]^x to see a specific effect.\n");

    int i = 0;
    for(const auto& [effectName, effect] : gConfig->effects) {
        i++;

        if(id != 0 && i != id) continue;

        player->printPaged(fmt::format("{})\tName: ^W{:<20}^x   Use Str: {}^x    Display: {}\n", i,
            effect.getName(), effect.usesStrength() ? "^gY" : "^rN", effect.getDisplay()));

        if(!all && i != id) continue;

        player->printPaged(fmt::format("\tOpposite Effect: {}\n", effect.getOppositeEffect()));
        player->printPaged(fmt::format("\tSelfAddStr: {}^x\n", effect.getSelfAddStr()));
        player->printPaged(fmt::format("\tRoomAddStr: {}^x\n", effect.getRoomAddStr()));
        player->printPaged(fmt::format("\tSelfDelStr: {}^x\n", effect.getSelfDelStr()));
        player->printPaged(fmt::format("\tRoomDelStr: {}^x\n", effect.getRoomDelStr()));
        player->printPaged(fmt::format("\tPulsed: {}^x\n", effect.isPulsed() ? "^GPulsed" : "Non-Pulsed"));
        if(effect.isPulsed())
            player->printPaged(fmt::format("\tPulse Delay: {}^x\n", effect.getPulseDelay()));
        player->printPaged(fmt::format("\tSpell: {}^x\n", effect.isSpell() ? "^GYes" : "^RNo"));
        player->printPaged(fmt::format("\tType: {}^x\n", effect.getType()));

        if(!effect.getApplyScript().empty())
            player->printPaged(fmt::format("\tApply Script: {}\n", effect.getApplyScript()));
        if(!effect.getPreApplyScript().empty())
            player->printPaged(fmt::format("\tPre-Apply Script: {}\n", effect.getPreApplyScript()));
        if(!effect.getPostApplyScript().empty())
            player->printPaged(fmt::format("\tPost-Apply Script: {}\n", effect.getPostApplyScript()));
        if(!effect.getComputeScript().empty())
            player->printPaged(fmt::format("\tCompute Script: {}\n", effect.getComputeScript()));
        if(!effect.getPulseScript().empty())
            player->printPaged(fmt::format("\tPulse Script: {}\n", effect.getPulseScript()));

        if(!effect.getUnApplyScript().empty())
            player->printPaged(fmt::format("\tUnApplyScript: {}\n", effect.getUnApplyScript()));
    }
    player->donePaging();

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
    diff = MIN<long>(MAX<long>(0, duration), diff);
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
                        std::clog << "Exit with no parent room!!!\n";
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
                std::clog << "Exit with no parent room!!!\n";
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
        myApplier = nullptr;

    return(success);
}

// End - EffectInfo functions


//*********************************************************************
//                      addEffect
//*********************************************************************


EffectInfo* MudObject::addEffect(const std::string& effect, long duration, int strength, MudObject* applier, bool show, const Creature* owner, bool keepApplier) {
    return(effects.addEffect(effect, duration, strength, applier, show, this, owner, keepApplier));
}

EffectInfo* Effects::addEffect(const std::string& effect, long duration, int strength, MudObject* applier, bool show, MudObject* pParent, const Creature* owner, bool keepApplier) {
    if(!gConfig->getEffect(effect))
        return(nullptr);
    auto* newEffect = new EffectInfo(effect, time(nullptr), duration, strength, pParent, owner);

    if(!newEffect->compute(applier)) {
        delete newEffect;
        return(nullptr);
    }


    if(strength != -2)
        newEffect->setStrength(strength);
    if(duration != -2)
        newEffect->setDuration(duration);

    return(addEffect(newEffect, show, nullptr, keepApplier));
}

EffectInfo* MudObject::addEffect(EffectInfo* newEffect, bool show, bool keepApplier) {
    return(effects.addEffect(newEffect, show, this, keepApplier));
}

EffectInfo* Effects::addEffect(EffectInfo* newEffect, bool show, MudObject* pParent, bool keepApplier) {
    if(pParent)
        newEffect->setParent(pParent);

    EffectInfo* oldEffect = getExactEffect(newEffect->getName());

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

EffectInfo* MudObject::addPermEffect(const std::string& effect, int strength, bool show) {
    return(effects.addEffect(effect, -1, strength, nullptr, show, this));
}

//*********************************************************************
//                      removeEffect
//*********************************************************************
// Remove the effect (Won't remove permanent effects if remPerm is false)

bool MudObject::removeEffect(const std::string& effect, bool show, bool remPerm, MudObject* fromApplier) {
    return(effects.removeEffect(effect, show, remPerm, fromApplier));
}

bool Effects::removeEffect(const std::string& effect, bool show, bool remPerm, MudObject* fromApplier) {
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
            (*it)->setOwner(nullptr);
    }
}

//*********************************************************************
//                      objectCanBestowEffect
//*********************************************************************

bool Effect::objectCanBestowEffect(std::string_view effect) {
    return( !effect.empty() &&
            effect != "vampirism" &&
            effect != "porphyria" &&
            effect != "lycanthropy"
    );
}

//*********************************************************************
//                      isEffected
//*********************************************************************

bool MudObject::isEffected(const std::string &effect, bool exactMatch) const {
    return(effects.isEffected(effect, exactMatch));
}

bool MudObject::isEffected(EffectInfo* effect) const {
    return(effects.isEffected(effect));
}

// We are effected if we have an effect with this name, or an effect with a base effect
// of this name

bool Effects::isEffected(const std::string &effect, bool exactMatch) const {
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
// Effect - Avian Aria, Base Effects = fly, levitate
// eff = fly

// Effect - Avian Aria, Base Effects = fly, levitate
// Effect Some Jackass's Flying Song.  Base effect = fly, levitate

const std::list<std::string>& Effect::getBaseEffects() {
    return(baseEffects);
}

//*********************************************************************
//                      hasPermEffect
//*********************************************************************

bool MudObject::hasPermEffect(std::string_view effect) const {
    EffectInfo* toCheck = effects.getEffect(effect);
    return(toCheck && toCheck->getDuration() == -1);
}

//*********************************************************************
//                      getEffect
//*********************************************************************

EffectInfo* MudObject::getEffect(std::string_view effect) const {
    return(effects.getEffect(effect));
}
// Returns the effect if we find one that the name matches or has
// the base effect mentioned

EffectInfo* Effects::getEffect(std::string_view effect) const {
    EffectInfo* toReturn = nullptr;
    for(const auto eff : effectList) {
        if(eff && (eff->getName() == effect || eff->hasBaseEffect(effect))) {
            if(!toReturn)
                toReturn = eff;
            else {
                // If we have something to return, compare it, we'll return the effect with the highest strength
                if(eff->getStrength() > toReturn->getStrength())
                    toReturn = eff;
            }
        }
    }
    return(toReturn);
}

//*********************************************************************
//                      getExactEffect
//*********************************************************************

EffectInfo* MudObject::getExactEffect(std::string_view effect) const {
    return(effects.getExactEffect(effect));
}

// Returns the effect with an exact name match
EffectInfo* Effects::getExactEffect(std::string_view effect) const {
    for(const auto eff : effectList) {
        if(eff && eff->getName() == effect)
            return(eff);
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
    EffectInfo* effect=nullptr;
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
    EffectInfo* effect=nullptr;
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
    EffectInfo* effect=nullptr;
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
//                      operator<<
//*********************************************************************

std::ostream& operator<<(std::ostream& out, const EffectInfo& effectInfo) {
    out << effectInfo.getName();
    return(out);
}

std::ostream& operator<<(std::ostream& out, EffectInfo* effectinfo) {
    if (effectinfo)
        out << *effectinfo;

    return (out);
}


//*********************************************************************
//                      getEffectsList
//*********************************************************************
// Return a list of effects

std::string Effects::getEffectsList() const {
    std::ostringstream effStr;

    effStr << "Effects: ";

    if(effectList.empty()) {
        effStr << "None";
    } else {
        effStr << join(effectList, ", ");
    }

    effStr << ".\n";

    std::string toPrint = effStr.str();

    return(toPrint);
}

//*********************************************************************
//                      getEffectsString
//*********************************************************************
// Used to print out what effects a creature is under

std::string Effects::getEffectsString(const Creature* viewer) {
    const Object* object=nullptr;
    std::ostringstream effStr;
    long t = time(nullptr);

    for(EffectInfo* effectInfo : effectList) {
        effectInfo->updateLastMod(t);

        effStr.setf(std::ios::right, std::ios::adjustfield);
        std::string display = effectInfo->getDisplayName();

        short count=0;
        int len = display.length();
        for(int i=0; i<len; i++) {
            if(display.at(i) == '^') {
                count += 2;
                i++;
            }
        }
        effStr << std::setw(38+count) << display << " - ";

        if(effectInfo->duration == -1)
            effStr << "Permanent!";
        else
            effStr << timeStr(effectInfo->duration);

        if( !viewer->isStaff() && effectInfo->getName() == "armor") 
            effStr << "  ^WStrength:^x " << effectInfo->getStrength();
    

        if(viewer->isStaff()) {
//          if(!effectInfo->getBaseEffect().empty())
//              effStr << " Base(" << effectInfo->getBaseEffect() << ")";
            effStr << "  ^WStrength:^x " << effectInfo->getStrength();
            if(effectInfo->getExtra()) {
                effStr << "  ^WExtra:^x " << effectInfo->getExtra();

                // extra information
                if(effectInfo->getName() == "illusion") {
                    const RaceData* race = gConfig->getRace(effectInfo->getExtra());
                    if(race)
                        effStr << " (" << race->getName() << ")";
                } else if(  (effectInfo->getDuration() == -1) && (
                    effectInfo->getName() == "wall-of-force" ||
                    effectInfo->getName() == "wall-of-fire" ||
                    effectInfo->getName() == "wall-of-thorns"
                ) ) {
                    // permanent walls are only down for a little bit
                    effStr << " (# pulses until reinstantiate)";
                }
            }
            if(effectInfo->getApplier()) {
                object = effectInfo->getApplier()->getAsConstObject();
                if(object)
                    effStr << "  ^WApplier:^x " << object->getName() << "^x";
            }
        }
        effStr << "\n";
    }

    return(effStr.str());
}

//*********************************************************************
//                      removeOppositeEffect
//*********************************************************************

bool MudObject::removeOppositeEffect(const EffectInfo *effect) {
    return(effects.removeOppositeEffect(effect));
}

bool Effects::removeOppositeEffect(const EffectInfo *effect) {
    auto* parentEffect = effect->getEffect();

    if(!parentEffect) return(false);
    if(parentEffect->getOppositeEffect().empty()) return(false);

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

    Player* killer=nullptr;
    bool online = true;

    if(!effect->getOwner().empty()) {
        killer = gServer->findPlayer(effect->getOwner());
        if(!killer) {
            if(loadPlayer(effect->getOwner().c_str(), &killer))
                online = false;
            else
                killer = nullptr;
        }
    }

    damage.set(Random::get(effect->getStrength() / 2, effect->getStrength() * 3 / 2));
    target->modifyDamage(nullptr, MAGICAL, damage, realm);

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

std::string Creature::doReplace(std::string fmt, const MudObject* actor, const MudObject* applier) const {
    const Creature* cActor = actor->getAsConstCreature();
    const Exit* xActor = actor->getAsConstExit();
    const Creature* cApplier = applier->getAsConstCreature();

    if(cActor) {
        boost::replace_all(fmt, "*ACTOR*", cActor->getCrtStr(this, CAP).c_str());
        boost::replace_all(fmt, "*LOW-ACTOR*", cActor->getCrtStr(this).c_str());
        boost::replace_all(fmt, "*A-HISHER*", cActor->hisHer());
        boost::replace_all(fmt, "*A-UPHISHER*", cActor->upHisHer());
    } else if(xActor) {
        boost::replace_all(fmt, "*ACTOR*", xActor->getCName());
        boost::replace_all(fmt, "*LOW-ACTOR*", xActor->getCName());
    }

    if(cApplier) {
        // Applier Possessive
        if(cActor == cApplier) {
            boost::replace_all(fmt, "*APPLIER-POS*", cApplier->upHisHer());
            boost::replace_all(fmt, "*LOW-APPLIER-POS*", cApplier->hisHer());
            boost::replace_all(fmt, "*APPLIER-SELF-POS*", "Your");
            boost::replace_all(fmt, "*LOW-APPLIER-SELF-POS*", "your");
        } else {
            boost::replace_all(fmt, "*APPLIER-POS*", std::string(cApplier->getCrtStr(this, CAP) + "'s").c_str());
            boost::replace_all(fmt, "*LOW-APPLIER-POS*", std::string(cApplier->getCrtStr(this) + "'s").c_str());
            boost::replace_all(fmt, "*APPLIER-SELF-POS*", std::string(cApplier->getCrtStr(this, CAP) + "'s").c_str());
            boost::replace_all(fmt, "*LOW-APPLIER-SELF-POS*", std::string(cApplier->getCrtStr(this) + "'s").c_str());
        }

        boost::replace_all(fmt, "*APPLIER*", cApplier->getCrtStr(this, CAP).c_str());
        boost::replace_all(fmt, "*LOW-APPLIER*", cApplier->getCrtStr(this).c_str());
        boost::replace_all(fmt, "*AP-HISHER*", cApplier->hisHer());
        boost::replace_all(fmt, "*AP-UPHISHER*", cApplier->upHisHer());
    }
    return(fmt);
}

//*********************************************************************
//                      effectEcho
//*********************************************************************

void Container::effectEcho(const std::string &fmt, const MudObject* actor, const MudObject* applier, Socket* ignore) {
    Socket* ignore2 = nullptr;
    if(actor->getAsConstCreature())
        ignore2 = actor->getAsConstCreature()->getSock();
    for(const Player* ply : players) {
        if(!ply || (ply->getSock() && (ply->getSock() == ignore || ply->getSock() == ignore2)) || ply->isUnconscious())
            continue;

        std::string toSend = ply->doReplace(fmt, actor, applier);
        (Streamable &) *ply << ColorOn << toSend << ColorOff << "\n";
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

bool EffectInfo::runScript(const std::string& pyScript, MudObject* applier) {

    // Legacy: Default action is return true, so play along with that
    if(pyScript.empty())
        return(true);

    try {
        auto locals = py::dict();
        auto effectModule = py::module::import("effectLib");
        locals["effectLib"] = effectModule;
        locals["effect"] = this;

        PythonHandler::addMudObjectToDictionary(locals, "actor", myParent);
        PythonHandler::addMudObjectToDictionary(locals, "applier", applier);

        return (gServer->runPythonWithReturn(pyScript, locals));
    }
    catch( pybind11::error_already_set& e) {
        PythonHandler::handlePythonError(e);
    }

    return(false);
}


//*********************************************************************
//                      pulseCreatureEffects
//*********************************************************************
// lastUserUpdate is set in updateUsers

void Server::pulseCreatureEffects(long t) {
    Monster *monster=nullptr;

    for (const auto& sock : sockets) {
        if(!sock.isConnected())
            continue;
        if(sock.hasPlayer())
            sock.getPlayer()->pulseEffects(t);
    }

    auto mIt = activeList.begin();
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
    if(!effects.effectList.empty())
        return(true);

    // any exit effects?
    for(Exit* exit : exits) {
        if(!exit->effects.effectList.empty())
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
//                      getPulseScript
//*********************************************************************

const std::string & Effect::getPulseScript() const {
    return(pulseScript);
}

//*********************************************************************
//                      getUnApplyScript
//*********************************************************************

const std::string & Effect::getUnApplyScript() const {
    return(unApplyScript);
}

//*********************************************************************
//                      getApplyScript
//*********************************************************************

const std::string & Effect::getApplyScript() const {
    return(applyScript);
}

//*********************************************************************
//                      getPreApplyScript
//*********************************************************************

const std::string & Effect::getPreApplyScript() const {
    return(preApplyScript);
}

//*********************************************************************
//                      getPostApplyScript
//*********************************************************************

const std::string & Effect::getPostApplyScript() const {
    return(postApplyScript);
}

//*********************************************************************
//                      getComputeScript
//*********************************************************************

const std::string & Effect::getComputeScript() const {
    return(computeScript);
}

//*********************************************************************
//                      getType
//*********************************************************************

const std::string & Effect::getType() const {
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
    return(useStrength);
}

//*********************************************************************
//                      getRoomDelStr
//*********************************************************************

const std::string & Effect::getRoomDelStr() const {
    return(roomDelStr);
}

//*********************************************************************
//                      getRoomAddStr
//*********************************************************************

const std::string & Effect::getRoomAddStr() const {
    return(roomAddStr);
}

//*********************************************************************
//                      getSelfDelStr
//*********************************************************************

const std::string & Effect::getSelfDelStr() const {
    return(selfDelStr);
}

//*********************************************************************
//                      getSelfAddStr
//*********************************************************************

const std::string & Effect::getSelfAddStr() const {
    return(selfAddStr);
}

//*********************************************************************
//                      getOppositeEffect
//*********************************************************************

const std::string & Effect::getOppositeEffect() const {
    return(oppositeEffect);
}

//*********************************************************************
//                      getDisplay
//*********************************************************************

const std::string & Effect::getDisplay() const {
    return(display);
}

//*********************************************************************
//                      hasBaseEffect
//*********************************************************************

bool EffectInfo::hasBaseEffect(std::string_view effect) const {
    return(myEffect->hasBaseEffect(effect));
}

//*********************************************************************
//                      hasBaseEffect
//*********************************************************************

bool Effect::hasBaseEffect(std::string_view effect) const {
    for(std::string_view be : baseEffects) {
        if(be == effect)
            return(true);
    }
    return(false);
}

//*********************************************************************
//                      getName
//*********************************************************************

const std::string & Effect::getName() const {
    return(name);
}

//*********************************************************************
//                      EffectInfo
//*********************************************************************
// TODO: Add applier here

EffectInfo::EffectInfo(const std::string &pName, time_t pLastMod, long pDuration, int pStrength, MudObject* pParent, const Creature* owner):
        name(pName), lastMod(pLastMod), lastPulse(pLastMod), duration(pDuration), strength(pStrength), myParent(pParent)
{
    myEffect = gConfig->getEffect(pName);
    if(!myEffect)
        throw std::runtime_error(fmt::format("Can't find effect '{}'", pName));
    setOwner(owner);
}

//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::EffectInfo() = default;

//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::EffectInfo(xmlNodePtr rootNode) {
    xmlNodePtr curNode = rootNode->children;

    while(curNode) {
            if(NODE_NAME(curNode, "Name")) xml::copyToString(name, curNode);
        else if(NODE_NAME(curNode, "Duration")) xml::copyToNum(duration, curNode);
        else if(NODE_NAME(curNode, "Strength")) xml::copyToNum(strength, curNode);
        else if(NODE_NAME(curNode, "Extra")) xml::copyToNum(extra, curNode);
        else if(NODE_NAME(curNode, "PulseModifier")) xml::copyToNum(pulseModifier, curNode);

        curNode = curNode->next;
    }

    lastPulse = lastMod = time(nullptr);
    myEffect = gConfig->getEffect(name);

    if(!myEffect) {
        throw std::runtime_error("Can't find effect listing " + name);
    }
}

//*********************************************************************
//                      EffectInfo
//*********************************************************************

EffectInfo::~EffectInfo() = default;

//*********************************************************************
//                      setParent
//*********************************************************************

void EffectInfo::setParent(MudObject* pParent) {
    myParent = pParent;
}

//*********************************************************************
//                      getEffect
//*********************************************************************

const Effect* EffectInfo::getEffect() const {
    return(myEffect);
}

//*********************************************************************
//                      getName
//*********************************************************************

std::string EffectInfo::getName() const {
    return(name);
}

//*********************************************************************
//                      getOwner
//*********************************************************************

std::string EffectInfo::getOwner() const {
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

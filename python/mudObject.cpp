/*
 * mudObject.cpp
 *   MudObject Python Library
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
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "container.hpp"
#include "creatures.hpp"                                       // for Creature
#include "fishing.hpp"
#include "mudObject.hpp"
#include "objects.hpp"
#include "rooms.hpp"
#include "socket.hpp"

namespace py = pybind11;
using namespace pybind11::literals;

void init_module_mudObject(py::module &m) {

    py::class_<MonsterSet> monsterSet(m, "MonsterSet");
    py::class_<PlayerSet>  playerSet(m, "PlayerSet");


    py::class_<MudObject> mudObject( m, "MudObject");
        mudObject
        .def("getName", &MudObject::getName)
        .def("isEffected", (bool (::MudObject::*) (const std::string&, bool) const)(&MudObject::isEffected), "effect"_a, "exactMatch"_a = false)
        .def("isEffected", (bool (::MudObject::*) (EffectInfo *) const)(&MudObject::isEffected), py::arg("effect"))
        .def("hasPermEffect", &MudObject::hasPermEffect)
        .def("addEffect", (EffectInfo* (::MudObject::*) (const std::string &, long int, int, MudObject*, bool, const Creature*, bool))&MudObject::addEffect,
             "effect"_a, "duration"_a=-2, "strength"_a=-2, "applier"_a=nullptr, "show"_a=(bool)(true),
             "owner"_a=py::none(), "keepApplier"_a=(bool)(false))
        .def("pulseEffects", &MudObject::pulseEffects)
        .def("removeOppositeEffect", &::MudObject::removeOppositeEffect)
        .def("getPlayer",&MudObject::getAsPlayer, py::return_value_policy::reference)
        .def("getMonster", &MudObject::getAsMonster, py::return_value_policy::reference)
        .def("getObject", &MudObject::getAsObject, py::return_value_policy::reference)
        .def("getExit",&MudObject::getAsExit, py::return_value_policy::reference)
        .def("equals", &MudObject::equals)
        .def("getId", &MudObject::getIdPython)
        ;

    py::class_<Container, MudObject>( m, "Container")
        .def_readwrite("monsters", &Container::monsters)
        .def_readwrite("players", &Container::players)
        .def("wake", &Container::wake)
        ;

    py::class_<Containable, MudObject>( m, "Containable")
        .def("getParent", &Containable::getParent, py::return_value_policy::reference)
        .def("addTo", &Containable::addTo)
        ;

    py::class_<BaseRoom, Container>( m, "BaseRoom")
        .def("flagIsSet", &BaseRoom::flagIsSet)
        .def("setFlag", &BaseRoom::setFlag)
        .def("findCreature", &BaseRoom::findCreaturePython, py::return_value_policy::reference)
        .def("hasMagicBonus", &BaseRoom::magicBonus)
        .def("killMortalObjects", &BaseRoom::killMortalObjects)
        .def("isForest", &BaseRoom::isForest)
        .def("setTempNoKillDarkmetal", &BaseRoom::setTempNoKillDarkmetal)
        .def("isSunlight", &BaseRoom::isSunlight)
        .def("getSize", &BaseRoom::getSize)
        .def("getFishing", &BaseRoom::getFishing)
    ;

    py::class_<Exit, MudObject>(m, "Exit")
        .def("getRoom", &Exit::getRoom, py::return_value_policy::reference)
    ;



    py::class_<Creature, Container, Containable>(m, "Creature")
        .def("send", &Creature::bPrintPython)
        .def("getCrtStr", &Creature::getCrtStr, "viewer"_a= nullptr, "ioFlags"_a=(int)(0), "num"_a=(int)(0) )
        .def("getParent", &Creature::getParent, py::return_value_policy::reference)
        .def("hisHer", &Creature::hisHer)
        .def("upHisHer", &Creature::upHisHer)
        .def("himHer", &Creature::himHer)
        .def("getRoom", &Creature::getRoomParent, py::return_value_policy::reference)
        .def("getTarget", &Creature::getTarget, py::return_value_policy::reference)
        .def("getDeity", &Creature::getDeity)
        .def("getClass", &Creature::getClass)
        .def("setDeathType", &Creature::setDeathType)
        .def("poisonedByMonster", &Creature::poisonedByMonster)
        .def("poisonedByPlayer", &Creature::poisonedByPlayer)
        .def("getLevel", &Creature::getLevel)
        .def("getAlignment", &Creature::getAlignment)
        .def("getArmor", &Creature::getArmor)
        .def("getDamageReduction", &Creature::getDamageReduction)
        .def("getExperience", &Creature::getExperience)
        .def("getPoisonedBy", &Creature::getPoisonedBy)
        .def("getClan", &Creature::getClan)
        .def("getType", &Creature::getType)
        .def("getRace", &Creature::getRace)
        .def("getSize", &Creature::getSize)
        .def("getAttackPower", &Creature::getAttackPower)
        .def("getDescription", &Creature::getDescription)
        .def("checkMp", &Creature::checkMp)
        .def("subMp", &Creature::subMp)
        .def("smashInvis", &Creature::smashInvis)
        .def("unhide", &Creature::unhide, "show"_a=(bool)(true) )
        .def("unmist", &Creature::unmist)
        .def("stun", &Creature::stun)
        .def("wake", &Creature::wake, "str"_a="", "noise"_a=(bool)(false) )

        .def("addStatModEffect", &Creature::addStatModEffect)
        .def("remStatModEffect", &Creature::remStatModEffect)
        .def("unApplyTongues", &Creature::unApplyTongues)
        .def("unBlind", &Creature::unBlind)
        .def("unSilence", &Creature::unSilence)
        .def("changeSize", &Creature::changeSize)

        .def("flagIsSet", &Creature::flagIsSet)
        .def("setFlag", &Creature::setFlag)
        .def("clearFlag", &Creature::clearFlag)
        .def("toggleFlag", &Creature::toggleFlag)

        .def("isPlayer", &Creature::isPlayer)
        .def("isMonster", &Creature::isMonster)

        .def("learnSpell", &Creature::setFlag)
        .def("forgetSpell", &Creature::clearFlag)
        .def("spellIsKnown", &Creature::spellIsKnown)

        .def("learnLanguage", &Creature::learnLanguage)
        .def("forgetLanguage", &Creature::forgetLanguage)
        .def("languageIsKnown", &Creature::languageIsKnown)

        .def("knowsSkill", &Creature::knowsSkill)
        .def("getSkillLevel", &Creature::getSkillLevel)
        .def("getSkillGained", &Creature::getSkillGained)
        .def("addSkill", &Creature::addSkill)
        .def("remSkill", &Creature::remSkill)
        .def("setSkill", &Creature::setSkill)

        .def("addExperience", &Creature::addExperience)
        .def("subExperience", &Creature::subExperience)
        .def("subAlignment", &Creature::subAlignment)

        .def("setClass", &Creature::setClass)
        .def("setClan", &Creature::setClan)
        .def("setRace", &Creature::setRace)
        .def("setLevel", &Creature::setLevel)

        .def("subAlignment", &Creature::subAlignment)
        .def("setSize", &Creature::setSize)
        .def("getWeight", &Creature::getWeight)
        .def("maxWeight", &Creature::maxWeight)

        .def("isVampire", &Creature::isNewVampire)
        .def("isWerewolf", &Creature::isNewWerewolf)
        .def("isUndead", &Creature::isUndead)
        .def("willBecomeVampire", &Creature::willBecomeVampire)
        .def("makeVampire", &Creature::makeVampire)
        .def("willBecomeWerewolf", &Creature::willBecomeWerewolf)
        .def("makeWerewolf", &Creature::makeWerewolf)
        .def("immuneCriticals", &Creature::immuneCriticals)
        .def("immuneToPoison", &Creature::immuneToPoison)
        .def("immuneToDisease", &Creature::immuneToDisease)

        .def("isRace", &Creature::isRace)
        .def("getSex", &Creature::getSex)

        .def("isBrittle", &Creature::isBrittle)
        .def("isBlind", &Creature::isBlind)
        .def("isUnconscious", &Creature::isUnconscious)
        .def("isBraindead", &Creature::isBraindead)

        .def("isHidden", &Creature::isHidden)
        .def("isInvisible", &Creature::isInvisible)

        .def("isWatcher", &Creature::isWatcher)
        .def("isStaff", &Creature::isStaff)
        .def("isCt", &Creature::isCt)
        .def("isDm", &Creature::isDm)
        .def("isAdm", &Creature::isAdm)
        .def("isPet", &Creature::isPet)
        .def_readonly( "hp", &Creature::hp )
        .def_readonly( "mp", &Creature::mp )
        .def_readonly( "strength", &Creature::strength )
        .def_readonly( "dexterity", &Creature::dexterity )
        .def_readonly( "constitution", &Creature::constitution )
        .def_readonly( "intelligence", &Creature::intelligence )
        .def_readonly( "piety", &Creature::piety )

        .def("delayedAction", &Creature::delayedAction)
        .def("delayedScript", &Creature::delayedScript)

    ;


    py::class_<Player, Creature> (m, "Player")
        .def("getSock", &Player::getSock, py::return_value_policy::reference)
        .def("customColorize", &Player::customColorize, "text"_a, "caret"_a=(bool)(true))
        .def("expToLevel", (unsigned long ( ::Player::* )() const)( &Player::expToLevel ) )
        .def("expForLevel", (std::string ( ::Player::* )( ) const)( &Player::expForLevel ) )
        .def("getCoinDisplay", &Player::getCoinDisplay)
        .def("getBankDisplay", &Player::getBankDisplay)
        .def("getWimpy", &Player::getWimpy)
        .def("getAfflictedBy", &Player::getAfflictedBy)
    ;

    py::class_<Monster, Creature> (m, "Monster")
        .def("addEnemy", &Monster::addEnemy)
        .def("adjustThreat", &Monster::adjustThreat)
        .def("customColorize", &Monster::customColorize, "text"_a, "caret"_a=(bool)(true) )
    ;

    py::class_<Object, Container, Containable> (m, "Object")
        .def("getType", &Object::getType)
        .def("getWearflag", &Object::getWearflag)
        .def("getShotsmax", &Object::getShotsMax)
        .def("getShotscur", &Object::getShotsCur)

        .def("flagIsSet", &Object::flagIsSet)
        .def("setFlag", &Object::setFlag)
        .def("clearFlag", &Object::clearFlag)
        .def("toggleFlag", &Object::toggleFlag)
        .def("getEffect", &Object::getEffect)
        .def("getEffectDuration", &Object::getEffectDuration)
        .def("getEffectStrength", &Object::getEffectStrength)

    ;

}

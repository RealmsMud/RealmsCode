/*
 * mud.cpp
 *   Mud Python Library
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


#include <pybind11/pybind11.h>       // for enum_, module, class_
#include <memory>                    // for unique_ptr

#include "commands.hpp"              // for doCastPython, isBadSocial, isGoo...
#include "config.hpp"                // for Config
#include "global.hpp"                // for CreatureClass, DeathType, religions
#include "monType.hpp"               // for mType, ANIMAL, ARACHNID, ASTRAL
#include "mudObjects/container.hpp"  // for Monste...
#include "mudObjects/creatures.hpp"  // for Creature
#include "mudObjects/objects.hpp"    // for Object
#include "mudObjects/rooms.hpp"      // for BaseRoom
#include "proto.hpp"                 // for dice, getConBonusPercentage, spa...
#include "random.hpp"                // for Random
#include "server.hpp"                // for Server
#include "socket.hpp"                // for Socket

namespace py = pybind11;


int pythonRand(int a, int b) {
    return (Random::get(a,b));
}


void init_module_mud(py::module &m) {
    py::class_<Config, std::unique_ptr<Config, py::nodelete>>(m, "Config")
        .def("getVersion", &Config::getVersion)
        .def("getMudName", &Config::getMudName)
        .def("getMudNameAndVersion",&Config::getMudNameAndVersion)
        .def("effectExists", &Config::effectExists)
        ;

    py::class_<Server, std::unique_ptr<Server, py::nodelete>>(m, "Server")
        .def("findPlayer", &Server::findPlayer, py::return_value_policy::reference)
        ;

    py::class_<Socket, std::shared_ptr<Socket>>(m, "Socket")
        .def("getPlayer", &Socket::getPlayer, py::return_value_policy::reference)
        .def("bprint", &Socket::bprintPython)
            ;

    py::enum_<CreatureClass>(m, "crtClasses")
            .value("ASSASSIN", CreatureClass::ASSASSIN)
            .value("BERSERKER", CreatureClass::BERSERKER)
            .value("CLERIC", CreatureClass::CLERIC)
            .value("FIGHTER", CreatureClass::FIGHTER)
            .value("MAGE", CreatureClass::MAGE)
            .value("PALADIN", CreatureClass::PALADIN)
            .value("RANGER", CreatureClass::RANGER)
            .value("THIEF", CreatureClass::THIEF)
            .value("PUREBLOOD", CreatureClass::PUREBLOOD)
            .value("MONK", CreatureClass::MONK)
            .value("DEATHKNIGHT", CreatureClass::DEATHKNIGHT)
            .value("DRUID", CreatureClass::DRUID)
            .value("LICH", CreatureClass::LICH)
            .value("WEREWOLF", CreatureClass::WEREWOLF)
            .value("BARD", CreatureClass::BARD)
            .value("ROGUE", CreatureClass::ROGUE)
            .value("BUILDER", CreatureClass::BUILDER)
            .value("CARETAKER", CreatureClass::CARETAKER)
            .value("DUNGEONMASTER", CreatureClass::DUNGEONMASTER)
            .value("CLASS_COUNT", CreatureClass::CLASS_COUNT)
            .export_values()
            ;


    py::enum_< religions>(m, "religions")
            .value("ATHEISM", ATHEISM)
            .value("ARAMON", ARAMON)
            .value("CERIS", CERIS)
            .value("ENOCH", ENOCH)
            .value("GRADIUS", GRADIUS)
            .value("ARES", ARES)
            .value("KAMIRA", KAMIRA)
            .value("LINOTHAN", LINOTHAN)
            .value("ARACHNUS", ARACHNUS)
            .value("MARA", MARA)
            .value("JAKAR", JAKAR)
            .value("MAX_DEITY", MAX_DEITY)
            .export_values()
            ;

    py::enum_< DeathType>(m, "DeathType")
            .value("DT_NONE", DT_NONE)
            .value("FALL", FALL)
            .value("POISON_MONSTER", POISON_MONSTER)
            .value("POISON_GENERAL", POISON_GENERAL)
            .value("DISEASE", DISEASE)
            .value("SMOTHER", SMOTHER)
            .value("FROZE", FROZE)
            .value("BURNED", BURNED)
            .value("DROWNED", DROWNED)
            .value("DRAINED", DRAINED)
            .value("ZAPPED", ZAPPED)
            .value("SHOCKED", SHOCKED)
            .value("WOUNDED", WOUNDED)
            .value("CREEPING_DOOM", CREEPING_DOOM)
            .value("SUNLIGHT", SUNLIGHT)
            .value("PIT", PIT)
            .value("BLOCK", BLOCK)
            .value("DART", DART)
            .value("ARROW", ARROW)
            .value("SPIKED_PIT", SPIKED_PIT)
            .value("FIRE_TRAP", FIRE_TRAP)
            .value("FROST", FROST)
            .value("ELECTRICITY", ELECTRICITY)
            .value("ACID", ACID)
            .value("ROCKS", ROCKS)
            .value("ICICLE_TRAP", ICICLE_TRAP)
            .value("SPEAR", SPEAR)
            .value("CROSSBOW_TRAP", CROSSBOW_TRAP)
            .value("VINES", VINES)
            .value("COLDWATER", COLDWATER)
            .value("EXPLODED", EXPLODED)
            .value("BOLTS", BOLTS)
            .value("SPLAT", SPLAT)
            .value("POISON_PLAYER", POISON_PLAYER)
            .value("BONES", BONES)
            .value("EXPLOSION", EXPLOSION)
            .value("PETRIFIED", PETRIFIED)
            .value("LIGHTNING", LIGHTNING)
            .value("WINDBATTERED", WINDBATTERED)
            .value("PIERCER", PIERCER)
            .value("ELVEN_ARCHERS", ELVEN_ARCHERS)
            .value("DEADLY_MOSS", DEADLY_MOSS)
            .value("THORNS", THORNS)
            .export_values()
            ;

    py::enum_< mType>(m, "mType")
            .value("INVALID", INVALID)
            .value("PLAYER", PLAYER)
            .value("MONSTER", MONSTER)
            .value("NPC", NPC)
            .value("HUMANOID", HUMANOID)
            .value("GOBLINOID", GOBLINOID)
            .value("MONSTROUSHUM", MONSTROUSHUM)
            .value("GIANTKIN", GIANTKIN)
            .value("ANIMAL", ANIMAL)
            .value("DIREANIMAL", DIREANIMAL)
            .value("INSECT", INSECT)
            .value("INSECTOID", INSECTOID)
            .value("ARACHNID", ARACHNID)
            .value("REPTILE", REPTILE)
            .value("DINOSAUR", DINOSAUR)
            .value("AUTOMATON", AUTOMATON)
            .value("AVIAN", AVIAN)
            .value("FISH", FISH)
            .value("PLANT", PLANT)
            .value("DEMON", DEMON)
            .value("DEVIL", DEVIL)
            .value("DRAGON", DRAGON)
            .value("BEAST", BEAST)
            .value("MAGICALBEAST", MAGICALBEAST)
            .value("GOLEM", GOLEM)
            .value("ETHEREAL", ETHEREAL)
            .value("ASTRAL", ASTRAL)
            .value("GASEOUS", GASEOUS)
            .value("ENERGY", ENERGY)
            .value("FAERIE", FAERIE)
            .value("DEVA", DEVA)
            .value("ELEMENTAL", ELEMENTAL)
            .value("PUDDING", PUDDING)
            .value("SLIME", SLIME)
            .value("UNDEAD", UNDEAD)
            .value("MAX_MOB_TYPES", MAX_MOB_TYPES)
            .export_values()
            ;

    // Misc Functions
    m.def("dice", &::dice);
    m.def("rand", &::pythonRand);
    m.def("spawnObjects", &::spawnObjects);
    m.def("isBadSocial", &::isBadSocial);
    m.def("isSemiBadSocial", &::isSemiBadSocial);
    m.def("isGoodSocial", &::isGoodSocial);
    m.def("getConBonusPercentage", &::getConBonusPercentage);
    //void doCastPython(std::shared_ptr<MudObject> caster, std::shared_ptr<Creature> target, std::string spell, int strength)
    m.def("doCast", &::doCastPython, py::arg("caster"), py::arg("target"), py::arg("spell"), py::arg("strength")=(int)(130) );

}
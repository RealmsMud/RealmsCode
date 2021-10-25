/*
 * mudEffects.cpp
 *   Effects Python Library
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

#include "creatures.hpp"                                       // for Creature
#include "effects.hpp"                                         // for Effect...
#include "mudObject.hpp"

namespace py = pybind11;

void init_module_effects(py::module &m) {

    py::class_<EffectInfo>( m, "EffectInfo")
        .def("add", &EffectInfo::add )
        .def("compute", &EffectInfo::compute)
        .def("pulse", &EffectInfo::pulse)
        .def("remove", &EffectInfo::remove)
        .def("runScript", &EffectInfo::runScript, py::arg("pyScript"), py::arg("applier")=0l )
        .def("getDisplayName", &EffectInfo::getDisplayName)
        .def("getDuration", &EffectInfo::getDuration)
        .def("getEffect", &EffectInfo::getEffect, py::return_value_policy::reference)

        .def("getExtra", &EffectInfo::getExtra)
        .def("getLastMod", &EffectInfo::getLastMod)
        .def("getName", &EffectInfo::getName)
        .def("getOwner", &EffectInfo::getOwner)
        .def("getParent", &EffectInfo::getParent, py::return_value_policy::reference)
        .def("getStrength", &EffectInfo::getStrength)
        .def("isCurse", &EffectInfo::isCurse)
        .def("isDisease", &EffectInfo::isDisease)
        .def("isPoison", &EffectInfo::isPoison)
        .def("isOwner", &EffectInfo::isOwner)
        .def("isPermanent", &EffectInfo::isPermanent)
        .def("willOverWrite", &EffectInfo::willOverWrite)

        .def("setDuration", &EffectInfo::setDuration)
        .def("setExtra", &EffectInfo::setExtra)
        .def("setOwner", &EffectInfo::setOwner)
        .def("setParent", &EffectInfo::setParent)
        .def("setStrength", &EffectInfo::setStrength)
        .def("updateLastMod", &EffectInfo::updateLastMod)
        ;

    py::class_<Effect>(m, "Effect")
        .def("getPulseScript", &Effect::getPulseScript)
        .def("getUnApplyScript", &Effect::getUnApplyScript)
        .def("getApplyScript", &Effect::getApplyScript)
        .def("getPreApplyScript", &Effect::getPreApplyScript)
        .def("getPostApplyScript", &Effect::getPostApplyScript)
        .def("getComputeScript", &Effect::getComputeScript)
        .def("getType", &Effect::getType)
        .def("getRoomDelStr", &Effect::getRoomDelStr)
        .def("getRoomAddStr", &Effect::getRoomAddStr)
        .def("getSelfDelStr", &Effect::getSelfDelStr)
        .def("getSelfAddStr", &Effect::getSelfAddStr)
        .def("getOppositeEffect", &Effect::getOppositeEffect)
        .def("getDisplay", &Effect::getDisplay)
        .def("getName", &Effect::getName)
        .def("getPulseDelay", &Effect::getPulseDelay)
        .def("isPulsed", &Effect::isPulsed)
        ;

}
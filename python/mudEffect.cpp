////
//// Created by jason on 9/26/21.
////
//
//#include <pybind11/pybind11.h>
////#include "commands.hpp"                                        // for doCast...
//#include "compile.h"                                           // for Py_fil...
//#include "config.hpp"                                          // for Config
//#include "container.hpp"                                       // for Monste...
//#include "creatures.hpp"                                       // for Creature
//#include "dictobject.h"                                        // for PyDict...
//#include "effects.hpp"                                         // for Effect...
//#include "exits.hpp"                                           // for Exit
//#include "fishing.hpp"                                         // for Fishing
//#include "global.hpp"                                          // for DeathType
//#include "import.h"                                            // for PyImpo...
//
//#include "monType.hpp"                                         // for mType
////#include "mudObject.hpp"                                       // for MudObject
//#include "object.h"                                            // for PyObject
//#include "objects.hpp"                                         // for Object
//#include "paths.hpp"                                           // for Python
//#include "proto.hpp"                                           // for broadcast
//#include "pyerrors.h"                                          // for PyErr_...
//#include "pylifecycle.h"                                       // for Py_Fin...
//#include "pythonHandler.hpp"                                   // for Python...
//#include "pythonrun.h"                                         // for PyErr_...
//#include "random.hpp"                                          // for Random
//#include "rooms.hpp"                                           // for BaseRoom
//#include "server.hpp"                                          // for Server
//#include "size.hpp"                                            // for Size
//#include "skills.hpp"                                          // for SkillInfo
//#include "socials.hpp"
//#include "socket.hpp"                                          // for Socket
//#include "stats.hpp"                                           // for Stat
//#include "unicodeobject.h"                                     // for PyUnic...
//
//namespace py = pybind11;
//
//PYBIND11_MODULE(mudEffect, m) {
//
//    py::class_<EffectInfo>( m, "EffectInfo")
//        .def("add", &EffectInfo::add )
//        .def("compute", &EffectInfo::compute)
//        .def("pulse", &EffectInfo::pulse)
//        .def("remove", &EffectInfo::remove)
//        .def("runScript", &EffectInfo::runScript, py::arg("pyScript"), py::arg("applier")=0l )
//        .def("getDisplayName", &EffectInfo::getDisplayName)
//        .def("getDuration", &EffectInfo::getDuration)
//        .def("getEffect", &EffectInfo::getEffect, py::return_value_policy::reference)
//
//        .def("getExtra", &EffectInfo::getExtra)
//        .def("getLastMod", &EffectInfo::getLastMod)
//        .def("getName", &EffectInfo::getName)
//        .def("getOwner", &EffectInfo::getOwner)
//        .def("getParent", &EffectInfo::getParent, py::return_value_policy::reference)
//        .def("getStrength", &EffectInfo::getStrength)
//        .def("isCurse", &EffectInfo::isCurse)
//        .def("isDisease", &EffectInfo::isDisease)
//        .def("isPoison", &EffectInfo::isPoison)
//        .def("isOwner", &EffectInfo::isOwner)
//        .def("isPermanent", &EffectInfo::isPermanent)
//        .def("willOverWrite", &EffectInfo::willOverWrite)
//
//        .def("setDuration", &EffectInfo::setDuration)
//        .def("setExtra", &EffectInfo::setExtra)
//        .def("setOwner", &EffectInfo::setOwner)
//        .def("setParent", &EffectInfo::setParent)
//        .def("setStrength", &EffectInfo::setStrength)
//        .def("updateLastMod", &EffectInfo::updateLastMod)
//        ;
//
//    py::class_<Effect>(m, "Effect")
//        .def("getPulseScript", &Effect::getPulseScript)
//        .def("getUnApplyScript", &Effect::getUnApplyScript)
//        .def("getApplyScript", &Effect::getApplyScript)
//        .def("getPreApplyScript", &Effect::getPreApplyScript)
//        .def("getPostApplyScript", &Effect::getPostApplyScript)
//        .def("getComputeScript", &Effect::getComputeScript)
//        .def("getType", &Effect::getType)
//        .def("getRoomDelStr", &Effect::getRoomDelStr)
//        .def("getRoomAddStr", &Effect::getRoomAddStr)
//        .def("getSelfDelStr", &Effect::getSelfDelStr)
//        .def("getSelfAddStr", &Effect::getSelfAddStr)
//        .def("getOppositeEffect", &Effect::getOppositeEffect)
//        .def("getDisplay", &Effect::getDisplay)
//        .def("getName", &Effect::getName)
//        .def("getPulseDelay", &Effect::getPulseDelay)
//        .def("isPulsed", &Effect::isPulsed)
//        ;
//
//}
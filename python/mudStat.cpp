/*
 * mudStat.cpp
 *   MudStat Python Library
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

#include <pybind11/pybind11.h>       // for class_, module
#include <pybind11/cast.h>           // for arg
#include <pybind11/detail/common.h>  // for constexpr_sum, pybind11
#include <pybind11/detail/descr.h>   // for operator+
#include <pybind11/pytypes.h>        // for getattr

#include "stats.hpp"                 // for Stat

namespace py = pybind11;

void init_module_stats(py::module &m) {
    py::class_<Stat>(m, "Stat")
        .def("getModifierAmt", &Stat::getModifierAmt)
        .def("adjust", &Stat::adjust, py::arg("amt"))
        .def("decrease", &Stat::decrease, py::arg("amt"))
        .def("getCur", &Stat::getCur, py::arg("recalc") = true)
        .def("getInitial", &Stat::getInitial)
        .def("getMax", &Stat::getMax)
        .def("increase", &Stat::increase, py::arg("amt"))
        .def("restore", &Stat::restore)
        .def("setCur", &Stat::setCur, py::arg("newCur"))
        .def("setInitial", &Stat::setInitial, py::arg("i"))
        .def("setMax", &Stat::setMax, py::arg("newMax"), py::arg("allowZero") = false);
}
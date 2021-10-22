//
// Created by jason on 9/26/21.
//

#include <pybind11/pybind11.h>

#include "object.h"                                            // for PyObject
#include "pyerrors.h"                                          // for PyErr_...
#include "pylifecycle.h"                                       // for Py_Fin...
#include "socket.hpp"                                          // for Socket
#include "stats.hpp"                                           // for Stat
#include "unicodeobject.h"                                     // for PyUnic...

namespace py = pybind11;

void init_mudStatModule(py::module &m) {
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
#include "lib.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>

#include <iostream>
#include <span>

namespace py = pybind11;

PYBIND11_MODULE(pylib, m){
m.def("hello", [](){std::cout << "Hello world!" << std::endl;});
m.def("foo", &foo);
m.def("sum", [](const py::array_t<float, py::array::c_style>& a){
    auto span = std::span<const float>(a.data(0), a.size());
    return sum(span);
});

}


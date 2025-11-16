#include "lib.hpp"

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <iostream>

namespace py = pybind11;

PYBIND11_MODULE(pylib, m){
m.def("hello", [](){std::cout << "Hello world!" << std::endl;});
m.def("foo", &foo);
m.def("sum", [](const py::array_t<float>& a){
    float result = 0.0;
    for(int i = 0; i < a.size(); ++i)
    {
      result += *(a.data() + i);
    }
    return result;
    });
}



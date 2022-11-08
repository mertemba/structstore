#include <pybind11/pybind11.h>
#include "example.hpp"
#include "structstore_python.hpp"

namespace py = pybind11;

PYBIND11_MODULE(structstore_example, m) {
    register_pystruct<Subsettings>(m, "Subsettings", nullptr);
    register_pystruct<Settings>(m, "Settings", nullptr);
}

#include <pybind11/pybind11.h>
#include "example.hpp"
#include <structstore_pybind.hpp>

namespace py = pybind11;

PYBIND11_MODULE(structstore_example, m) {
    stst::register_pystruct<Subsettings>(m, "Subsettings", nullptr);
    stst::register_pystruct<Settings>(m, "Settings", nullptr);
}

#include <nanobind/nanobind.h>
#include <structstore/stst_py.hpp>
#include "mystruct0.hpp"

namespace nb = nanobind;

NB_MODULE(MODULE_NAME, m) {
    auto frame_cls = nb::class_<Frame>(m, "Frame");
    frame_cls.def(nb::init());
    stst::py::register_struct_type<Frame>(frame_cls);
}

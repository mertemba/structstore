#include <nanobind/nanobind.h>
#include <structstore/stst_py.hpp>
#include "mystruct0.hpp"
#include "mystruct1.hpp"

namespace nb = nanobind;

NB_MODULE(MODULE_NAME, m) {
    auto frame_cls = nb::class_<Frame>(m, "Frame");
    frame_cls.def(nb::init());
    stst::py::register_struct_type<Frame>(frame_cls);

    auto track_cls = nb::class_<Track>(m, "Track");
    track_cls.def(nb::init());
    stst::py::register_struct_type<Track>(track_cls);
}

#include <nanobind/nanobind.h>
#include <structstore/stst_py.hpp>
#include "mystruct0.hpp"
#include "mystruct1.hpp"

namespace nb = nanobind;

NB_MODULE(MODULE_NAME, m) {
    auto frame_cls = nb::class_<Frame::Ref>(m, "Frame");
    stst::py::register_struct_type<Frame::Ref>(frame_cls);

    auto track_cls = nb::class_<Track::Ref>(m, "Track");
    stst::py::register_struct_type<Track::Ref>(track_cls);
}

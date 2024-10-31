#include <nanobind/nanobind.h>
#include <structstore/structstore.hpp>
#include <structstore/stst_py.hpp>

namespace nb = nanobind;
namespace stst = structstore;

struct Frame : public stst::Struct<Frame> {
    double t = 0.0;

    Frame() : Frame(stst::static_alloc) {}

    explicit Frame(stst::MiniMalloc& mm_alloc) : Struct(mm_alloc) {
        store("t", t);
    }

    Frame(const Frame& other) : Frame() {
        *this = other;
    }

    Frame(Frame&& other) : Frame() {
        *this = std::move(other);
    }

    Frame& operator=(const Frame& other) {
        store = other.store;
        return *this;
    }

    Frame& operator=(Frame&& other) {
        store = std::move(other.store);
        return *this;
    }
};

NB_MODULE(MODULE_NAME, m) {
    stst::typing::register_type<Frame>("Frame");
    auto frame_cls = nb::class_<Frame>(m, "Frame");
    frame_cls.def(nb::init());
    stst::py::register_struct_type<Frame>(frame_cls);
}

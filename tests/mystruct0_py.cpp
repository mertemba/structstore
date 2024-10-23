#include <nanobind/nanobind.h>
#include <structstore/structstore.hpp>
#include <structstore/stst_py.hpp>

namespace nb = nanobind;
namespace stst = structstore;

struct Frame : public stst::Struct {
    double t = 0.0;

    Frame() : Frame(stst::static_alloc) {}

    explicit Frame(stst::MiniMalloc& mm_alloc) : Struct(mm_alloc) {
        STST_LOG_DEBUG() << "constructing Frame at " << this;
        store("t", t);
    }

    Frame(const Frame& other) : Frame() {
        STST_LOG_DEBUG() << "copy-constructing Frame at " << this << " from " << &other;
        *this = other;
    }

    Frame(Frame&& other) : Frame() {
        STST_LOG_DEBUG() << "move-constructing Frame at " << this << " from " << &other;
        *this = std::move(other);
    }

    ~Frame() { STST_LOG_DEBUG() << "deconstructing Frame at " << this; }

    Frame& operator=(const Frame& other) {
        STST_LOG_DEBUG() << "copying Frame from " << &other << " to " << this;
        if (&other == this) { throw std::runtime_error("copying to itself, wtf?"); }
        store = other.store;
        return *this;
    }

    Frame& operator=(Frame&& other) {
        STST_LOG_DEBUG() << "moving Frame from " << &other << " to " << this;
        if (&other == this) { throw std::runtime_error("moving to itself, wtf?"); }
        store = std::move(other.store);
        return *this;
    }

    bool operator==(const Frame& other) const { return stst::Struct::operator==(other); }
};

template<>
YAML::Node structstore::to_yaml(const Frame& frame) {
    return structstore::to_yaml(frame.get_store());
}

NB_MODULE(MODULE_NAME, m) {
    stst::typing::register_type<Frame>("Frame");
    auto frame_cls = nb::class_<Frame>(m, "Frame");
    frame_cls.def(nb::init());
    stst::py::register_struct_type<Frame>(frame_cls);
}

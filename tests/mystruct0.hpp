#include <structstore/structstore.hpp>

namespace stst = structstore;

struct Frame : public stst::Struct<Frame> {
    static const stst::TypeInfo& type_info;

    double t = 0.0;
    bool flag = false;

    Frame() : Frame(stst::static_alloc) {}

    explicit Frame(stst::MiniMalloc& mm_alloc) : Struct(mm_alloc) {
        store("t", t);
        store("flag", flag);
    }

    Frame(const Frame& other) : Frame() { *this = other; }

    Frame(Frame&& other) : Frame() { *this = std::move(other); }

    Frame& operator=(const Frame& other) {
        store = other.store;
        return *this;
    }

    Frame& operator=(Frame&& other) {
        store = std::move(other.store);
        return *this;
    }
};

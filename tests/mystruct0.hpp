#ifndef STST_MYSTRUCT0_HPP
#define STST_MYSTRUCT0_HPP

#include <structstore/structstore.hpp>

namespace stst = structstore;

struct Frame : public stst::Struct<Frame> {
    static const stst::TypeInfo& type_info;

    double t = 0.0;
    bool flag = false;
    double* t_ptr = &t;

    Frame() : Frame(stst::static_alloc) {}

    explicit Frame(stst::MiniMalloc& mm_alloc) : Struct(mm_alloc) {
        store("t", t);
        store("flag", flag);
        store("t_ptr", t_ptr);
    }

    Frame(const Frame& other) : Frame() { *this = other; }

    Frame(Frame&& other) : Frame() { *this = std::move(other); }

    Frame& operator=(const Frame& other) {
        store = other.store;
        t_ptr = &t;
        return *this;
    }

    Frame& operator=(Frame&& other) {
        store = std::move(other.store);
        t_ptr = &t;
        return *this;
    }
};

#endif

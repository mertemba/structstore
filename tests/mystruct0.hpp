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
        field_map.store_ref("t", t);
        field_map.store_ref("flag", flag);
        field_map.store_ref("t_ptr", t_ptr);
    }

    Frame(const Frame& other) : Frame() { *this = other; }

    Frame(Frame&& other) : Frame() { *this = std::move(other); }

    Frame& operator=(const Frame& other) {
        field_map = other.field_map;
        t_ptr = &t;
        return *this;
    }
};

#endif

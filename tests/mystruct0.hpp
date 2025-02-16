#ifndef STST_MYSTRUCT0_HPP
#define STST_MYSTRUCT0_HPP

#include <structstore/structstore.hpp>

namespace stst = structstore;

struct Frame : public stst::Struct<Frame> {
    static const stst::TypeInfo& type_info;

    double t = 0.0;
    bool flag = false;
    stst::OffsetPtr<double> t_ptr = &t;

    explicit Frame(stst::SharedAlloc& sh_alloc) : Struct(sh_alloc) {
        store_ref("t", t);
        store_ref("flag", flag);
        store_ref("t_ptr", t_ptr);
    }

    Frame& operator=(const Frame& other) {
        copy_from(other);
        t_ptr = &t;
        return *this;
    }
};

#endif
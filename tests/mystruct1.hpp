#ifndef STST_MYSTRUCT1_HPP
#define STST_MYSTRUCT1_HPP

#include "mystruct0.hpp"
#include <structstore/structstore.hpp>

namespace stst = structstore;

struct Track : public stst::Struct<Track> {
    static const stst::TypeInfo& type_info;

    Frame frame1;
    Frame frame2;
    stst::OffsetPtr<Frame> frame_ptr = &frame1;

    explicit Track(stst::SharedAlloc& sh_alloc)
        : Struct(sh_alloc), frame1{sh_alloc}, frame2{sh_alloc} {
        store_ref("frame1", frame1);
        store_ref("frame2", frame2);
        store_ref("frame_ptr", frame_ptr);
    }

    Track& operator=(const Track& other) {
        copy_from(other);
        frame_ptr = &frame1;
        return *this;
    }
};

#endif
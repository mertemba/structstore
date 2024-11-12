#ifndef STST_MYSTRUCT1_HPP
#define STST_MYSTRUCT1_HPP

#include "mystruct0.hpp"
#include <structstore/structstore.hpp>

namespace stst = structstore;

struct Track : public stst::Struct<Track> {
    static const stst::TypeInfo& type_info;

    Frame frame1;
    Frame frame2;
    Frame* frame_ptr = &frame1;

    Track() : Track(stst::static_alloc) {}

    explicit Track(stst::MiniMalloc& mm_alloc)
        : Struct(mm_alloc), frame1{mm_alloc}, frame2{mm_alloc} {
        store("frame1", frame1);
        store("frame2", frame2);
        store("frame_ptr", frame_ptr);
    }

    Track(const Track& other) : Track() { *this = other; }

    Track(Track&& other) : Track() { *this = std::move(other); }

    Track& operator=(const Track& other) {
        store = other.store;
        return *this;
    }

    Track& operator=(Track&& other) {
        store = std::move(other.store);
        return *this;
    }
};

#endif

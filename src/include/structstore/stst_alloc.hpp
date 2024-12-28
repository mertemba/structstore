#ifndef STST_ALLOC_HPP
#define STST_ALLOC_HPP

#include "structstore/mini_malloc.hpp"
#include "structstore/stst_callstack.hpp"
#include "structstore/stst_lock.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace structstore {

class MiniMalloc {
    using byte = uint8_t;
    static constexpr size_t ALIGN = 8;

    // member variables
    mini_malloc* mm;
    SpinMutex mutex;
    byte* const buffer;
    const size_t blocksize;

public:
    MiniMalloc(void* buffer, size_t size) : mm{nullptr}, buffer{(byte*) buffer}, blocksize{size} {
        if (buffer == nullptr) { return; }
        mm = init_mini_malloc(buffer, size);
    }

    ~MiniMalloc() noexcept(false) { mm_assert_all_freed(mm); }

    MiniMalloc() = delete;

    MiniMalloc(MiniMalloc&&) = delete;

    MiniMalloc(const MiniMalloc&) = delete;

    MiniMalloc& operator=(MiniMalloc&&) = delete;

    MiniMalloc& operator=(const MiniMalloc&) = delete;

    bool init() { return true; }

    void dispose() {}

    void* allocate(size_t field_size) {
        if (field_size == 0) { field_size = ALIGN; }
        ScopedLock<true> lock{mutex};
        void* ptr = mm_alloc(mm, field_size);
        if (ptr == nullptr) {
            std::ostringstream str;
            str << "insufficient space in mm_alloc region, requested: " << field_size;
            Callstack::throw_with_trace<std::runtime_error>(str.str());
        }
        return ptr;
    }

    void deallocate(void* ptr) {
        ScopedLock<true> lock{mutex};
        mm_free(mm, ptr);
    }

    bool is_owned(const void* ptr) const {
        if (ptr == nullptr) {
#ifndef NDEBUG
            Callstack::warn_with_trace("checked pointer is null");
#endif
            return false;
        }
        if (ptr < buffer || ptr >= buffer + blocksize) {
#ifndef NDEBUG
            Callstack::warn_with_trace("checked pointer is outside arena");
#endif
            return false;
        }
        return true;
    }
};

extern MiniMalloc static_alloc;

template<typename T = char>
class StlAllocator {
    template<typename U>
    friend class StlAllocator;

    MiniMalloc& mm_alloc;

public:
    using value_type = T;

    explicit StlAllocator(MiniMalloc& a) : mm_alloc(a) {}

    template<typename U>
    StlAllocator(const StlAllocator<U>& other) : mm_alloc(other.mm_alloc) {}

    T* allocate(std::size_t n) { return static_cast<T*>(mm_alloc.allocate(n * sizeof(T))); }

    void deallocate(T* p, std::size_t) { mm_alloc.deallocate(p); }

    // defined in stst_typing.hpp
    inline void construct(T* p);

    void construct(T* p, T&& other) {
        construct(p);
        *p = std::move(other);
    }

    void construct(T* p, const T& other) {
        construct(p);
        *p = other;
    }

    template<typename U>
    bool operator==(StlAllocator<U> const& rhs) const {
        return &mm_alloc == &rhs.mm_alloc;
    }

    template<typename U>
    bool operator!=(StlAllocator<U> const& rhs) const {
        return &mm_alloc != &rhs.mm_alloc;
    }

    MiniMalloc& get_alloc() { return mm_alloc; }
};
} // namespace structstore

#endif

#ifndef STST_ALLOC_HPP
#define STST_ALLOC_HPP

#include "ankerl/unordered_dense.h"
#include "structstore/mini_malloc.hpp"
#include "structstore/stst_callstack.hpp"
#include "structstore/stst_lock.hpp"
#include "structstore/stst_offsetptr.hpp"
#include "structstore/stst_utils.hpp"

#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace structstore {

class StringStorage;

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class SharedAlloc {
    using byte = uint8_t;
    static constexpr size_t ALIGN = 8;

    // member variables
    OffsetPtr<mini_malloc> mm;
    SpinMutex mutex;
    const uint32_t blocksize;
    OffsetPtr<StringStorage> string_storage;

public:
    SharedAlloc(void* buffer, size_t size);

    ~SharedAlloc() noexcept(false);

    SharedAlloc() = delete;

    SharedAlloc(SharedAlloc&&) = delete;

    SharedAlloc(const SharedAlloc&) = delete;

    SharedAlloc& operator=(SharedAlloc&&) = delete;

    SharedAlloc& operator=(const SharedAlloc&) = delete;

    bool init() { return true; }

    void dispose() {}

    template<typename T = void>
    T* allocate(size_t field_size = sizeof(T)) {
        if (field_size == 0) { field_size = ALIGN; }
        ScopedLock<true> lock{mutex};
        void* ptr = mm_allocate(mm.get(), field_size);
        if (ptr == nullptr) {
            std::ostringstream str;
            str << "insufficient space in sh_alloc region, requested: " << field_size;
            Callstack::throw_with_trace(str.str());
        }
        assert((size_t) ptr % ALIGN == 0);
        STST_LOG_DEBUG() << "allocating " << typeid(T).name() << " at " << ptr;
        return (T*) ptr;
    }

    void deallocate(const void* ptr) {
        STST_LOG_DEBUG() << "deallocating at " << ptr;
        ScopedLock<true> lock{mutex};
        mm_free(mm.get(), ptr);
    }

    bool is_owned(const void* ptr) const {
        if (ptr == nullptr) {
#ifndef NDEBUG
            Callstack::warn_with_trace("checked pointer is null");
#endif
            return false;
        }
        if (ptr < (byte*) mm.get() || ptr >= (byte*) mm.get() + blocksize) {
#ifndef NDEBUG
            Callstack::warn_with_trace("checked pointer is outside arena");
#endif
            return false;
        }
        return true;
    }

    inline StringStorage& strings() { return *string_storage.get(); }

    inline const StringStorage& strings() const { return *string_storage.get(); }
};

extern SharedAlloc& static_alloc;

class StructStore;

template<typename T = char>
class StlAllocator {
    template<typename U>
    friend class StlAllocator;

    SharedAlloc& sh_alloc;

public:
    using value_type = T;
    using pointer = OffsetPtr<T, int64_t>;

    explicit StlAllocator(SharedAlloc& a) : sh_alloc(a) {}

    template<typename U>
    StlAllocator(const StlAllocator<U>& other) : sh_alloc(other.sh_alloc) {}

    T* allocate(std::size_t n) { return static_cast<T*>(sh_alloc.allocate(n * sizeof(T))); }

    void deallocate(T* p, std::size_t) { sh_alloc.deallocate(p); }
    void deallocate(const OffsetPtr<T, int32_t>& p, std::size_t) { sh_alloc.deallocate(p.get()); }
    void deallocate(const OffsetPtr<T, int64_t>& p, std::size_t) { sh_alloc.deallocate(p.get()); }

    void construct(T* p) {
        if constexpr (std::is_constructible_v<T, SharedAlloc&> || std::is_same_v<T, StructStore>) {
            new (p) T(sh_alloc);
        } else if constexpr (std::is_constructible_v<T, const StlAllocator<T>&>) {
            new (p) T(StlAllocator<T>{sh_alloc});
        } else {
            new (p) T();
        }
    }

    void construct(T* p, T&& other) {
        new (p) T(std::move(other));
    }

    void construct(T* p, const T& other) {
        new (p) T(other);
    }

    template<typename U>
    bool operator==(StlAllocator<U> const& rhs) const {
        return &sh_alloc == &rhs.sh_alloc;
    }

    template<typename U>
    bool operator!=(StlAllocator<U> const& rhs) const {
        return &sh_alloc != &rhs.sh_alloc;
    }

    SharedAlloc& get_alloc() { return sh_alloc; }
};

using shr_string = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;

using shr_string_idx = uint16_t;

template<class T>
using shr_vector = std::vector<T, StlAllocator<T>>;

template<class K, class T, class H = ankerl::unordered_dense::hash<K>>
using shr_unordered_map = ankerl::unordered_dense::map<K, T, H, std::equal_to<K>,
                                                       StlAllocator<std::pair<const K, T>>>;

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class StringStorage {
    shr_unordered_map<shr_string, shr_string_idx> map;
    shr_vector<shr_string> data;
    mutable SpinMutex mutex;

public:
    StringStorage(SharedAlloc& sh_alloc);

    shr_string_idx internalize(const std::string& str, SharedAlloc& sh_alloc);

    shr_string_idx get_idx(const std::string& str, SharedAlloc& sh_alloc) const;

    const shr_string* get(shr_string_idx idx) const;
};

} // namespace structstore

#endif

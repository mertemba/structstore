#ifndef STST_OFFSETPTR_HPP
#define STST_OFFSETPTR_HPP

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace structstore {

template<typename T>
class OffsetPtr {
    static constexpr std::ptrdiff_t empty_val = 1;
    std::ptrdiff_t offset = empty_val;

    static std::ptrdiff_t ptr_to_int(const void* ptr) {
        return reinterpret_cast<const std::ptrdiff_t>(ptr);
    }

    static T* int_to_T_ptr(std::ptrdiff_t offset) { return reinterpret_cast<T*>(offset); }

public:
    // using element_type = T;
    using pointer = T*;
    // using const_pointer = const T*;
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using reference = std::conditional_t<std::is_void_v<T>, int, T>&;
    // using const_reference = std::conditional_t<std::is_void_v<T>, const int, const T>&;
    using iterator_category = std::random_access_iterator_tag;
    // template<class U>
    // using rebind = OffsetPtr<U>;

    // static functions

    static OffsetPtr pointer_to(reference t) { return OffsetPtr(&t); }

    // constructors and assignment operators

    explicit OffsetPtr() {}

    OffsetPtr(T* t) { *this = t; }

    OffsetPtr(const OffsetPtr& other) : OffsetPtr(other.get()) {}

    OffsetPtr(OffsetPtr&& other) : OffsetPtr(other.get()) {}

    OffsetPtr& operator=(T* t) {
        if (t == nullptr) {
            offset = empty_val;
        } else {
            offset = ptr_to_int(t) - ptr_to_int(this);
        }
        return *this;
    }

    OffsetPtr& operator=(const OffsetPtr& other) { return *this = other.get(); }

    OffsetPtr& operator=(OffsetPtr&& other) { return *this = other.get(); }

    ~OffsetPtr() = default;

    // explicit accessors

    T* get() const {
        if (offset == empty_val) return nullptr;
        return int_to_T_ptr(ptr_to_int(this) + offset);
    }

    // explicit operator T*() { return get(); }

    reference operator*() const {
        assert(*this);
        return *get();
    }

    T* operator->() const { return get(); }

    reference operator[](std::size_t idx) const {
        assert(*this);
        return get()[idx];
    }

    explicit operator bool() const { return get() != nullptr; }

    // implicit conversions

    // special case for std::basic_string
    operator char*() const { return get(); }

    operator OffsetPtr<void>() const { return OffsetPtr<void>(get()); }

    operator OffsetPtr<const T>() const { return OffsetPtr<const T>(get()); }

    // comparison operators

    bool operator==(const OffsetPtr<const T>& other) const { return get() == other.get(); }

    bool operator==(const OffsetPtr<std::remove_const_t<T>>& other) const {
        return get() == other.get();
    }

    bool operator!=(const OffsetPtr<T>& other) const { return get() != other.get(); }

    // arithmetic operators

    OffsetPtr operator+(std::ptrdiff_t shift) const { return OffsetPtr(get() + shift); }

    OffsetPtr operator-(std::ptrdiff_t shift) const { return OffsetPtr(get() - shift); }

    std::ptrdiff_t operator-(const OffsetPtr& other) const { return get() - other.get(); }

    OffsetPtr& operator++() { return *this = get() + 1; }

    OffsetPtr& operator--() { return *this = get() - 1; }
};

} // namespace structstore

#endif

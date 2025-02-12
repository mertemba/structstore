#ifndef STST_OFFSETPTR_HPP
#define STST_OFFSETPTR_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace structstore {

template<typename diff_type = int32_t>
class OffsetPtrBase {};

template<typename T, typename diff_type = int32_t>
class OffsetPtr : public OffsetPtrBase<diff_type> {
public:
    static_assert(std::is_signed_v<diff_type>);

    // using element_type = T;
    using pointer = OffsetPtr<T, int64_t>;
    // using const_pointer = const T*;
    using difference_type = diff_type;
    using value_type = T;
    using reference = std::conditional_t<std::is_void_v<T>, int, T>&;
    // using const_reference = std::conditional_t<std::is_void_v<T>, const int, const T>&;
    using iterator_category = std::random_access_iterator_tag;
    // template<class U>
    // using rebind = OffsetPtr<U>;

    static constexpr diff_type empty_val = 1;
    static constexpr ptrdiff_t max_ptrdiff_val = ((1ll << (8 * sizeof(diff_type) - 2)) - 1);

    diff_type offset = empty_val;

    // static functions

    static std::ptrdiff_t ptr_to_int(const void* ptr) {
        return reinterpret_cast<const std::ptrdiff_t>(ptr);
    }

    static T* int_to_T_ptr(std::ptrdiff_t offset) { return reinterpret_cast<T*>(offset); }

    static OffsetPtr pointer_to(reference t) { return OffsetPtr(&t); }

    // constructors and assignment operators

    explicit OffsetPtr() {}

    OffsetPtr(T* t) { *this = t; }

    OffsetPtr(const OffsetPtr& other) : OffsetPtr(other.get()) {}

    OffsetPtr(OffsetPtr&& other) : OffsetPtr(other.get()) {}

    OffsetPtr& operator=(T* t) {
        if (t == nullptr) {
            this->offset = this->empty_val;
        } else {
            ptrdiff_t diff = ptr_to_int(t) - ptr_to_int(this);
            assert(diff < this->max_ptrdiff_val);
            assert(diff > -this->max_ptrdiff_val);
            this->offset = diff;
        }
        return *this;
    }

    OffsetPtr& operator=(const OffsetPtr& other) { return *this = other.get(); }

    OffsetPtr& operator=(OffsetPtr&& other) { return *this = other.get(); }

    ~OffsetPtr() = default;

    // explicit accessors

    T* get() const {
        if (this->offset == this->empty_val) return nullptr;
        return int_to_T_ptr(ptr_to_int(this) + (ptrdiff_t) this->offset);
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

    operator OffsetPtr<void, diff_type>() const { return OffsetPtr<void, diff_type>(get()); }

    operator OffsetPtr<const T, diff_type>() const { return OffsetPtr<const T, diff_type>(get()); }

    // comparison operators

    bool operator==(const OffsetPtr<const T, diff_type>& other) const {
        return get() == other.get();
    }

    bool operator==(const OffsetPtr<std::remove_const_t<T>, diff_type>& other) const {
        return get() == other.get();
    }

    bool operator==(const T* other) const { return get() == other; }

    bool operator==(std::remove_const_t<T>* other) const { return get() == other; }

    bool operator==(std::nullptr_t) const { return !*this; }

    bool operator!=(const OffsetPtr& other) const { return get() != other.get(); }

    bool operator!=(std::nullptr_t) const { return !!*this; }

    // arithmetic operators

    OffsetPtr operator+(diff_type shift) const { return OffsetPtr(get() + shift); }

    OffsetPtr operator-(diff_type shift) const { return OffsetPtr(get() - shift); }

    diff_type operator-(const OffsetPtr& other) const { return get() - other.get(); }

    OffsetPtr& operator++() { return *this = get() + 1; }

    OffsetPtr& operator--() { return *this = get() - 1; }
};

} // namespace structstore

#endif

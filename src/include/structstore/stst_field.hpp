#ifndef STST_FIELD_HPP
#define STST_FIELD_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_callstack.hpp"
#include "structstore/stst_offsetptr.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <type_traits>
#include <yaml-cpp/yaml.h>

namespace structstore {

template<bool managed>
class FieldMap;

template<bool managed>
class FieldAccess;

class Field;

class FieldView {
    friend class Field;

    void* data;
    type_hash_t type_hash;

    void assert_nonempty() const {
        if (!data) { throw std::runtime_error("field is not yet initialized!"); }
    }

    void assert_empty() const {
        if (data) {
            throw std::runtime_error("field is replaced/deleted while still initialized!");
        }
    }

public:
    // constructor, assignment, destructor

    template<typename T>
    explicit FieldView(T& data) : data{&data}, type_hash{typing::get_type_hash<T>()} {
        static_assert(typing::is_field_type<T>);
    }

    explicit FieldView(void* data, type_hash_t type_hash) : data{data}, type_hash{type_hash} {}

    // FieldTypeBase utility functions

    void to_text(std::ostream& os) const;

    YAML::Node to_yaml() const;

    void check(const SharedAlloc& sh_alloc, const FieldTypeBase& parent_field) const;

    bool operator==(const FieldView& other) const {
        if (!data) { return !other.data; }
        if (type_hash != other.type_hash) { return false; }
        return typing::get_type(type_hash).cmp_equal_fn(data, other.data);
    }

    // query functions

    [[nodiscard]] bool empty() const { return !data; }

    [[nodiscard]] type_hash_t get_type_hash() const { return type_hash; }

    template<typename T>
    T& get() const {
        CallstackEntry entry{"FieldView::get()"};
        static_assert(typing::is_field_type<T>, "field accessed with invalid type");
        assert_nonempty();
        if (type_hash != typing::get_type_hash<T>()) {
            Callstack::throw_with_trace("field accessed with wrong type");
        }
        return *(T*) data;
    }
};

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class Field {
protected:
    friend class FieldView;
    template<bool managed>
    friend class FieldAccess;
    template<typename T>
    friend class FieldRef;

    OffsetPtr<void> data;
    type_hash_t type_hash;

    inline void assert_nonempty() const { view().assert_nonempty(); }

    inline void assert_empty() const { view().assert_empty(); }

    template<bool>
    friend class structstore::FieldMap;

    void construct_copy_from(SharedAlloc& sh_alloc, const Field& other,
                             const FieldTypeBase* parent_field);

    void copy_from(SharedAlloc& sh_alloc, const Field& other);

    void move_from(Field& other);

    void clear(SharedAlloc& sh_alloc) {
        if (data) {
            const auto& type_info = typing::get_type(type_hash);
            STST_LOG_DEBUG() << "deconstructing field " << type_info.name << " at " << data.get();
            type_info.destructor_fn(sh_alloc, data.get());
            sh_alloc.deallocate(data.get());
        }
        data = nullptr;
        type_hash = 0;
    }

    void clear_unmanaged() {
        data = nullptr;
        type_hash = 0;
    }

    template<typename T>
    void replace_data(void* new_data, SharedAlloc& sh_alloc) {
        assert_empty();
        clear(sh_alloc);
        data = new_data;
        type_hash = typing::get_type_hash<T>();
        STST_LOG_DEBUG() << "replacing field data with " << new_data << ", type "
                         << typing::get_type<T>().name;
    }

    template<typename T>
    T& get_or_construct(SharedAlloc& sh_alloc, const FieldTypeBase* parent_field) {
        if (empty()) {
            T* ptr = sh_alloc.allocate<T>();
            STST_LOG_DEBUG() << "allocating at " << ptr;
            const auto& type_info = typing::get_type<T>();
            STST_LOG_DEBUG() << "constructing field " << type_info.name << " at " << ptr;
            STST_LOG_DEBUG() << "parent field at " << parent_field;
            type_info.constructor_fn(sh_alloc, ptr, parent_field);
            replace_data<T>(ptr, sh_alloc);
        }
        return get<T>();
    }

public:
    // constructor, assignment, destructor

    Field(void* data, type_hash_t type_hash) : data(data), type_hash(type_hash) {}

    Field() : Field{nullptr, 0} {}

    template<typename T>
    explicit Field(T* data) : Field{data, typing::get_type_hash<T>()} {}

    Field(Field&& other) noexcept : Field() { *this = std::move(other); }

    Field& operator=(Field&& other) noexcept {
        std::swap(data, other.data);
        std::swap(type_hash, other.type_hash);
        return *this;
    }

    Field(const Field&) = delete;
    Field& operator=(const Field&) = delete;

    ~Field() noexcept(false) {
        if (data) {
            throw std::runtime_error("internal error: Field was not cleaned up, type is " +
                                     typing::get_type(type_hash).name);
        }
    }

    template<typename T>
    void set_data(T* data) {
        assert_empty();
        this->data = data;
        this->type_hash = typing::get_type_hash<T>();
    }

    // FieldTypeBase utility functions

    inline FieldView view() const { return FieldView{data.get(), type_hash}; }

    inline void to_text(std::ostream& os) const { view().to_text(os); }

    inline YAML::Node to_yaml() const { return view().to_yaml(); }

    inline void check(const SharedAlloc& sh_alloc, const FieldTypeBase& parent_field) const {
        CallstackEntry entry{"structstore::Field::check()"};
        stst_assert(sh_alloc.is_owned(this));
        view().check(sh_alloc, parent_field);
    }

    inline bool operator==(const Field& other) const { return view() == other.view(); }

    inline bool operator!=(const Field& other) const { return !(*this == other); }

    // query operations

    [[nodiscard]] bool empty() const { return !data; }

    [[nodiscard]] type_hash_t get_type_hash() const { return type_hash; }

    template<typename T>
    inline T& get() const {
        return view().get<T>();
    }

    template<typename T>
    operator T&() const {
        return get<T>();
    }

    // assignment operations

    template<typename T>
    Field& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }
};

class StructStoreShared;

class String;

class List;

template<bool managed>
class FieldAccess {
    Field& field;
    SharedAlloc& sh_alloc;
    const FieldTypeBase* parent_field;

public:
    FieldAccess() = delete;

    explicit FieldAccess(Field& field, SharedAlloc& sh_alloc, const FieldTypeBase* parent_field)
        : field(field), sh_alloc(sh_alloc), parent_field(parent_field) {}

    FieldAccess(const FieldAccess& other) = default;

    FieldAccess& operator=(const FieldAccess& other) = delete;

    template<typename T>
    T& get() {
        static_assert(typing::is_field_type<T>);
        if constexpr (managed) {
            return field.get_or_construct<T>(sh_alloc, parent_field);
        } else {
            return field.get<T>();
        }
    }

    ::structstore::String& get_str();

    template<typename T>
    operator T&() {
        return get<T>();
    }

    FieldAccess<true> operator[](const std::string& name) { return get<StructStore>()[name]; }

    FieldAccess<true> operator[](size_t idx) { return get<List>()[idx]; }

    operator FieldAccess<false>() { return FieldAccess<false>{field, sh_alloc, parent_field}; }

    FieldAccess<true> to_managed_access() {
        return FieldAccess<true>{field, sh_alloc, parent_field};
    }

    template<typename T>
    FieldAccess& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    template<std::size_t N>
    FieldAccess& operator=(const char (&value)[N]) {
        get<String>() = value;
        return *this;
    }

    FieldAccess& operator=(const std::string& str) {
        get<String>() = str;
        return *this;
    }

    Field& get_field() { return field; }

    SharedAlloc& get_alloc() { return sh_alloc; }

    [[nodiscard]] type_hash_t get_type_hash() const { return field.get_type_hash(); }

    void clear() { field.clear(sh_alloc); }
};

// this class resides in local stack or heap memory;
// it manages a reference to a field within a SharedAlloc
template<typename T>
class FieldRef {
    static_assert(std::is_base_of_v<FieldTypeBase, T>);
    T& data;
    SharedAlloc* sh_alloc;

public:
    // construct in static memory arena
    static void create_in_place(FieldRef* ref) {
        T* t = static_alloc.allocate<T>();
        StlAllocator<T>(static_alloc).construct(t);
        new (ref) FieldRef{*t};
        ref->sh_alloc = &static_alloc;
    }

    // construct in static memory arena
    static FieldRef create() {
        T* t = static_alloc.allocate<T>();
        StlAllocator<T>(static_alloc).construct(t);
        FieldRef ref{*t};
        ref.sh_alloc = &static_alloc;
        return std::move(ref);
    }

    // construct with existing data, do not manage pointer
    FieldRef(T& data) : data{data}, sh_alloc{nullptr} {}

    // move from other is valid
    FieldRef(FieldRef&& other) : data{other.data}, sh_alloc{other.sh_alloc} {
        // we take ownership of the data; the other wrapper does not manage anymore
        other.sh_alloc = nullptr;
    }

    // move from field is valid
    FieldRef(Field&& other, SharedAlloc& sh_alloc) : data{other.get<T>()}, sh_alloc{&sh_alloc} {
        // we take ownership of the data
        other.clear_unmanaged();
    }

    // empty construction, copy, and assignment are invalid
    FieldRef() = delete;
    FieldRef(const FieldRef&) = delete;
    FieldRef operator=(FieldRef&&) = delete;
    FieldRef operator=(const FieldRef&) = delete;

    ~FieldRef() {
        // if we created the object, we need to clean up
        if (sh_alloc) {
            data.~T();
            sh_alloc->deallocate(&data);
        }
    }

    // access functions are only valid on l-value FieldRefs
    // to avoid dangling references
    T* operator->() & { return &data; }
    T& operator*() & { return data; }

    void check() & {
        CallstackEntry entry{"structstore::FieldRef::check()"};
        if (sh_alloc) { stst_assert(sh_alloc->is_owned(&data)); }
        data.check();
    }
};

// unwrapping FieldRef:

template<typename W>
struct Unwrapper {
    using T = W;
    T& t;
    Unwrapper(W& w) : t{w} {}
};

template<typename T_>
struct Unwrapper<FieldRef<T_>> {
    using T = T_;
    T& t;
    Unwrapper(FieldRef<T>& w) : t{*w} {}
};

// specialization for Unwrapper<StructStoreShared> in stst_shared.hpp

template<typename W>
auto& unwrap(W& w) {
    static_assert(std::is_same_v<W, std::remove_const_t<W>>);
    static_assert(std::is_same_v<W, std::remove_reference_t<W>>);
    return Unwrapper(w).t;
}

template<typename W>
const auto& unwrap(const W& w) {
    static_assert(std::is_same_v<W, std::remove_const_t<W>>);
    static_assert(std::is_same_v<W, std::remove_reference_t<W>>);
    return Unwrapper((W&) w).t;
}

template<typename W>
struct unwrap_type {
    static_assert(std::is_same_v<W, std::remove_const_t<W>>);
    static_assert(std::is_same_v<W, std::remove_reference_t<W>>);
    using T = typename Unwrapper<W>::T;
};

template<typename T>
using unwrap_type_t = typename unwrap_type<T>::T;

static_assert(std::is_same_v<unwrap_type_t<int>, int>);
static_assert(std::is_same_v<unwrap_type_t<OffsetPtr<int>>, OffsetPtr<int>>);

// wrapping in FieldRef:

template<typename T, class Enable = void>
struct RefWrapper {
    using W = T&;
};

template<typename T>
struct RefWrapper<T, std::enable_if_t<std::is_base_of_v<FieldTypeBase, T>>> {
    using W = FieldRef<T>;
};

template<typename T>
decltype(auto) ref_wrap(T& t) {
    static_assert(std::is_same_v<T, std::remove_const_t<T>>);
    static_assert(std::is_same_v<T, std::remove_reference_t<T>>);
    return typename RefWrapper<T>::W(t);
}

template<typename T>
struct wrap_type {
    static_assert(std::is_same_v<T, std::remove_const_t<T>>);
    static_assert(std::is_same_v<T, std::remove_reference_t<T>>);
    using W = typename RefWrapper<T>::W;
};

template<typename T>
using wrap_type_w = typename wrap_type<T>::W;

static_assert(std::is_same_v<wrap_type_w<int>, int&>);
static_assert(std::is_same_v<wrap_type_w<OffsetPtr<int>>, OffsetPtr<int>&>);
}

#endif

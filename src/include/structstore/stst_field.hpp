#ifndef STST_FIELD_HPP
#define STST_FIELD_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <yaml-cpp/yaml.h>

namespace structstore {

template<bool managed>
class FieldMap;

class FieldView;

template<bool managed>
class FieldAccess;

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class Field {
protected:
    friend class FieldView;
    template<bool managed>
    friend class FieldAccess;

    OffsetPtr<void> data;
    type_hash_t type_hash;

    void assert_nonempty() const {
        if (!data) { throw std::runtime_error("field is not yet initialized!"); }
    }

    void assert_empty() const {
        if (data) {
            throw std::runtime_error("field is replaced/deleted while still initialized!");
        }
    }

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

    void to_text(std::ostream& os) const;

    YAML::Node to_yaml() const;

    void check(const SharedAlloc& sh_alloc, const FieldTypeBase& parent_field) const;

    bool operator==(const Field& other) const {
        if (!data) { return !other.data; }
        if (type_hash != other.type_hash) { return false; }
        return typing::get_type(type_hash).cmp_equal_fn(data.get(), other.data.get());
    }

    inline bool operator!=(const Field& other) const { return !(*this == other); }

    // query operations

    [[nodiscard]] bool empty() const { return !data; }

    [[nodiscard]] type_hash_t get_type_hash() const { return type_hash; }

    template<typename T>
    T& get() const {
        static_assert(typing::is_field_type<T>, "field accessed with invalid type");
        assert_nonempty();
        if (type_hash != typing::get_type_hash<T>()) {
            throw std::runtime_error("field accessed with wrong type");
        }
        return *(T*) data.get();
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

class FieldView {
    Field field;

public:
    template<typename T>
    explicit FieldView(T& data) : field{&data} {}

    Field& operator*() {
        return field;
    }

    Field* operator->() {
        return &field;
    }

    ~FieldView() {
        field.clear_unmanaged();
    }
};

template<>
FieldView::FieldView(structstore::StructStoreShared& data);

class String;

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
}

#endif

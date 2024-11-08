#ifndef STST_FIELD_HPP
#define STST_FIELD_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <yaml-cpp/yaml.h>

namespace structstore {

class StructStore;

class Field {
private:
    void* data;
    uint64_t type_hash;

    void assert_nonempty() const {
        if (data == nullptr) {
            throw std::runtime_error("field is not yet initialized!");
        }
    }

    void assert_empty() const {
        if (data != nullptr) {
            throw std::runtime_error("field is replaced/deleted while still initialized!");
        }
    }

    friend class structstore::StructStore;

    void constr_copy_from(MiniMalloc& mm_alloc, const Field& other);

    void copy_from(MiniMalloc& mm_alloc, const Field& other);

    void move_from(Field& other);

public:
    Field() : data(nullptr), type_hash(0) {}

    template<typename T>
    explicit Field(T* data)
            : data(data), type_hash(typing::get_type_hash<T>()) {}

    explicit Field(void* data, uint64_t type_hash)
        : data(data), type_hash(type_hash) {}

    Field(Field&& other) noexcept: Field() {
        *this = std::move(other);
    }

    Field(const Field&) = delete;

    ~Field() noexcept(false) {
        if (data) {
            throw std::runtime_error("field was not cleaned up, type is " +
                                     typing::get_type(type_hash).name);
        }
    }

    [[nodiscard]] bool empty() const {
        return !data;
    }

    void clear(MiniMalloc& mm_alloc) {
        if (data) {
            const auto& type_info = typing::get_type(type_hash);
            STST_LOG_DEBUG() << "deconstructing field " << type_info.name << " at " << data;
            type_info.destructor_fn(mm_alloc, data);
            STST_LOG_DEBUG() << "deallocating at " << data;
            mm_alloc.deallocate(data);
        }
        data = nullptr;
        type_hash = 0;
    }

    void clear_unmanaged() {
        data = nullptr;
        type_hash = 0;
    }

    template<typename T>
    void replace_data(void* new_data, MiniMalloc& mm_alloc) {
        assert_empty();
        clear(mm_alloc);
        data = new_data;
        type_hash = typing::get_type_hash<T>();
        STST_LOG_DEBUG() << "replacing field data with " << new_data << ", type "
                         << typing::get_type(type_hash).name;
    }

    Field& operator=(Field&& other) noexcept {
        std::swap(data, other.data);
        std::swap(type_hash, other.type_hash);
        return *this;
    }

    Field& operator=(const Field&) = delete;

    [[nodiscard]] uint64_t get_type_hash() const {
        return type_hash;
    }

    void to_text(std::ostream& os) const;

    friend std::ostream& operator<<(std::ostream& os, const Field& field) {
        field.to_text(os);
        return os;
    }

    YAML::Node to_yaml() const;

    template<typename T>
    T& get() const {
        assert_nonempty();
        if (type_hash != typing::get_type_hash<T>()) {
            throw std::runtime_error("field accessed with wrong type");
        }
        return *(T*) data;
    }

    template<typename T>
    operator T&() const {
        return get<T>();
    }

    template<typename T>
    Field& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    void check(MiniMalloc& mm_alloc) const {
        if (data != nullptr) {
            try_with_info("in field data ptr: ", mm_alloc.assert_owned(data););
            const TypeInfo& type_info = typing::get_type(type_hash);
            try_with_info("in field data content: ", type_info.check_fn(mm_alloc, data););
        }
    }

    bool operator==(const Field& other) const {
        if (data == nullptr) {
            return other.data == nullptr;
        }
        if (type_hash != other.type_hash) {
            return false;
        }
        return typing::get_type(type_hash).cmp_equal_fn(data, other.data);
    }

    bool operator!=(const Field& other) const {
        return !(*this == other);
    }
};

class StructStoreShared;

class FieldView {
    Field field;

public:
    template<typename T>
    explicit FieldView(T& data) : field{&data} {}

    template<typename T>
    explicit FieldView(T& data, uint64_t type_hash) : field{&data, type_hash} {}

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
}

#endif

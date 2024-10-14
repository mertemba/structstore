#ifndef STST_FIELD_HPP
#define STST_FIELD_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <iostream>

#include <yaml-cpp/yaml.h>

namespace structstore {

class StructStoreField;

template<>
std::ostream& to_text(std::ostream&, const StructStoreField&);

template<>
YAML::Node to_yaml(const StructStoreField&);

class StructStoreField {
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

public:
    StructStoreField() : data(nullptr), type_hash(0) {}

    template<typename T>
    explicit StructStoreField(T* data)
            : data(data), type_hash(typing::get_type_hash<T>()) {}

    explicit StructStoreField(void* data, uint64_t type_hash)
        : data(data), type_hash(type_hash) {}

    StructStoreField(StructStoreField&& other) noexcept: StructStoreField() {
        *this = std::move(other);
    }

    StructStoreField(const StructStoreField&) = delete;

    ~StructStoreField() noexcept(false) {
        if (data) {
            throw std::runtime_error("field was not cleaned up, type is " + typing::get_type_name(type_hash));
        }
    }

    [[nodiscard]] bool empty() const {
        return !data;
    }

    void clear(MiniMalloc& mm_alloc) {
        if (data) {
            const typing::DestructorFn<>& destructor = typing::get_destructor(type_hash);
            destructor(mm_alloc, data);
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
    }

    StructStoreField& operator=(StructStoreField&& other) noexcept {
        std::swap(data, other.data);
        std::swap(type_hash, other.type_hash);
        return *this;
    }

    StructStoreField& operator=(const StructStoreField&) = delete;

    [[nodiscard]] uint64_t get_type_hash() const {
        return type_hash;
    }

    template<typename T>
    friend std::ostream& structstore::to_text(std::ostream&, const T&);

    template<typename T>
    friend YAML::Node structstore::to_yaml(const T&);

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& field) {
        return to_text(os, field);
    }

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
    StructStoreField& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    void check(MiniMalloc& mm_alloc) const {
        if (data != nullptr) {
            try_with_info("in field data ptr: ", mm_alloc.assert_owned(data););
            const typing::CheckFn<>& check = typing::get_check(type_hash);
            try_with_info("in field data content: ", check(mm_alloc, data););
        }
    }
};

class StructStoreShared;

class FieldView {
    StructStoreField field;

public:
    template<typename T>
    explicit FieldView(T& data) : field{&data} {}

    template<typename T>
    explicit FieldView(T& data, uint64_t type_hash) : field{&data, type_hash} {}

    StructStoreField& operator*() {
        return field;
    }

    ~FieldView() {
        field.clear_unmanaged();
    }
};

template<>
FieldView::FieldView(structstore::StructStoreShared& data);
}

#endif

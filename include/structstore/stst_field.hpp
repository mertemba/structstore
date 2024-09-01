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

std::ostream& operator<<(std::ostream& os, const StructStoreField& self);

YAML::Node to_yaml(const StructStoreField& self);

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

    void clear(MiniMalloc& mm_alloc, bool managed = true) {
        if (data && managed) {
            const typing::DestructorFn<>& destructor = typing::get_destructor(type_hash);
            destructor(mm_alloc, data);
            mm_alloc.deallocate(data);
        }
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

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self);

    friend YAML::Node to_yaml(const StructStoreField& self);

    template<typename T>
    T& get() const {
        assert_nonempty();
        if (type_hash != typing::get_type_hash<T>()) {
            throw std::runtime_error("field accessed with wrong type");
        }
        return *(T*) data;
    }

    template<typename T>
    operator T&() {
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

}

#endif

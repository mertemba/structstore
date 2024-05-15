#ifndef STST_FIELD_HPP
#define STST_FIELD_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_alloc.hpp"
#include "structstore/stst_typing.hpp"

#include <iostream>
#include <typeindex>

#include <yaml-cpp/yaml.h>

namespace structstore {

class StructStoreField;

std::ostream& operator<<(std::ostream& os, const StructStoreField& self);

YAML::Node to_yaml(const StructStoreField& self);

class StructStoreField {
private:
    void* data;
    std::type_index type;

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
    StructStoreField() : data(nullptr), type(typeid(nullptr_t)) {}

    template<typename T>
    explicit StructStoreField(T* data) : data(data), type(typeid(T)) {}

    StructStoreField(StructStoreField&& other) noexcept: StructStoreField() {
        *this = std::move(other);
    }

    StructStoreField(const StructStoreField&) = delete;

    ~StructStoreField() noexcept(false) {
        if (data) {
            throw std::runtime_error("field was not cleaned up, type is " + std::to_string((int) type));
        }
    }

    [[nodiscard]] bool empty() const {
        return !data;
    }

    void clear(MiniMalloc& mm_alloc) {
        if (data) {
            const typing::DestructorFn<>& destructor = typing::get_destructor(type);
            destructor(mm_alloc, data);
            mm_alloc.deallocate(data);
            data = nullptr;
        }
    }

    template<typename T>
    void replace_data(void* new_data, MiniMalloc& mm_alloc) {
        if (data) {
            mm_alloc.deallocate(data);
        }
        data = new_data;
        type = typeid(T);
    }

    StructStoreField& operator=(StructStoreField&& other) noexcept {
        std::swap(data, other.data);
        std::swap(type, other.type);
        return *this;
    }

    StructStoreField& operator=(const StructStoreField&) = delete;

    [[nodiscard]] std::type_index get_type() const {
        return type;
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self);

    friend YAML::Node to_yaml(const StructStoreField& self);

    template<typename T>
    T& get() const {
        assert_nonempty();
        if (type != typeid(T)) {
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
};

}

#endif

#ifndef STST_FIELD_HPP
#define STST_FIELD_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_alloc.hpp"

#include <iostream>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace structstore {

class StructStore;

class List;

class Matrix;

enum class FieldTypeValue : uint8_t {
    EMPTY,
    INT,
    DOUBLE,
    STRING,
    BOOL,
    STRUCT,
    LIST,
    MATRIX,
};

void serialize_text(std::ostream& os, FieldTypeValue type, void* data);
YAML::Node serialize_yaml(FieldTypeValue type, void* data);

template<typename T, class Enable = void>
struct FieldType;

template<>
struct FieldType<int> {
    static constexpr auto value = FieldTypeValue::INT;
};

template<>
struct FieldType<double> {
    static constexpr auto value = FieldTypeValue::DOUBLE;
};

template<>
struct FieldType<structstore::string> {
    static constexpr auto value = FieldTypeValue::STRING;
};

template<>
struct FieldType<bool> {
    static constexpr auto value = FieldTypeValue::BOOL;
};

template<>
struct FieldType<StructStore> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

template<class T>
struct FieldType<T, std::enable_if_t<std::is_base_of_v<StructStore, T>>> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

template<>
struct FieldType<List> {
    static constexpr auto value = FieldTypeValue::LIST;
};

template<>
struct FieldType<Matrix> {
    static constexpr auto value = FieldTypeValue::MATRIX;
};

void destruct(StructStore& store);

void destruct(List& store);

void destruct(Matrix& store);

class StructStoreField {
private:
    void* data;
    FieldTypeValue type;

    void assert_nonempty() const {
        if (data == nullptr || type == FieldTypeValue::EMPTY) {
            throw std::runtime_error("field is not yet initialized!");
        }
    }

    void assert_empty() const {
        if (data != nullptr || type != FieldTypeValue::EMPTY) {
            throw std::runtime_error("field is replaced/deleted while still initialized!");
        }
    }

public:
    StructStoreField() : data(nullptr), type(FieldTypeValue::EMPTY) {}

    template<typename T>
    explicit StructStoreField(T* data) : data(data), type(FieldType<T>::value) {}

    StructStoreField(StructStoreField&& other) noexcept: StructStoreField() {
        *this = std::move(other);
    }

    StructStoreField(const StructStoreField&) = delete;

    ~StructStoreField() noexcept(false) {
        if (data) {
            throw std::runtime_error("field was not cleaned up, type is " + std::to_string((int) type));
        }
    }

    void clear(MiniMalloc& mm_alloc) {
        if (data) {
            switch (type) {
                case FieldTypeValue::INT:
                case FieldTypeValue::DOUBLE:
                case FieldTypeValue::BOOL:
                case FieldTypeValue::EMPTY:
                    break;
                case FieldTypeValue::STRING:
                    get<structstore::string>().~string();
                    break;
                case FieldTypeValue::STRUCT:
                    destruct(get<StructStore>());
                    break;
                case FieldTypeValue::LIST:
                    destruct(get<List>());
                    break;
                case FieldTypeValue::MATRIX:
                    destruct(get<Matrix>());
                    break;
                default:
                    throw std::runtime_error("unknown type in StructStore");
            }
            mm_alloc.deallocate(data);
            data = nullptr;
        }
        type = FieldTypeValue::EMPTY;
    }

    template<typename T>
    void replace_data(T* new_data, MiniMalloc& mm_alloc) {
        if (data) {
            mm_alloc.deallocate(data);
        }
        data = new_data;
        type = FieldType<T>::value;
    }

    StructStoreField& operator=(StructStoreField&& other) noexcept {
        std::swap(data, other.data);
        std::swap(type, other.type);
        return *this;
    }

    StructStoreField& operator=(const StructStoreField&) = delete;

    [[nodiscard]] FieldTypeValue get_type() const {
        return type;
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self);

    friend YAML::Node to_yaml(const StructStoreField& self);

    template<typename T>
    T& get() const {
        assert_nonempty();
        if (FieldType<T>::value != type) {
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

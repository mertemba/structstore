#ifndef STRUCTSTORE_FIELD_HPP
#define STRUCTSTORE_FIELD_HPP

#include <iostream>
#include <type_traits>
#include <yaml-cpp/yaml.h>

#include "structstore_alloc.hpp"

class StructStoreBase;

template<class, size_t>
class StructStore;

enum class FieldTypeValue : uint8_t {
    INT,
    STRING,
    ARENA_STR,
    BOOL,
    STRUCT,
};

template<typename T, class Enable = void>
struct FieldType;

template<>
struct FieldType<int> {
    static constexpr auto value = FieldTypeValue::INT;
};

template<>
struct FieldType<std::string> {
    static constexpr auto value = FieldTypeValue::STRING;
};

template<>
struct FieldType<arena_str> {
    static constexpr auto value = FieldTypeValue::ARENA_STR;
};

template<>
struct FieldType<bool> {
    static constexpr auto value = FieldTypeValue::BOOL;
};

template<>
struct FieldType<StructStoreBase> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

template<class T>
struct FieldType<T, std::enable_if_t<std::is_base_of_v<StructStoreBase, T>>> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

class StructStoreField {
private:
    void* data;

public:
    const FieldTypeValue type;

    template<typename T>
    StructStoreField(T& field) : data(&field), type(FieldType<T>::value) {}

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self) {
        ser_text_funcs[self.type](os, self.data);
        return os;
    }

    friend YAML::Node to_yaml(const StructStoreField& self) {
        return ser_yaml_funcs[self.type](self.data);
    }

    template<typename T>
    operator T&() {
        if (type != FieldType<T>::value) {
            throw std::runtime_error("field is not of type " + std::string(typeid(T).name()));
        }
        return *(T*) data;
    }

    template<typename T>
    StructStoreField& operator=(const T& value) {
        (T&) *this = value;
        return *this;
    }
};

#endif

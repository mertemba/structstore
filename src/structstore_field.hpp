#ifndef STRUCTSTORE_FIELD_HPP
#define STRUCTSTORE_FIELD_HPP

#include <iostream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "hashstring.hpp"

namespace structstore {

class StructStore;

enum class FieldTypeValue : uint8_t {
    INT,
    DOUBLE,
    STRING,
    BOOL,
    STRUCT,
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

class StructStoreField {
private:
    void* data;

public:
    const FieldTypeValue type;

    template<typename T>
    StructStoreField(T& field) : data(&field), type(FieldType<T>::value) {}

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self) {
        serialize_text(os, self.type, self.data);
        return os;
    }

    friend YAML::Node to_yaml(const StructStoreField& self) {
        return serialize_yaml(self.type, self.data);
    }

    template<typename T>
    T& get() {
        return *(T*) data;
    }
};

}

#endif

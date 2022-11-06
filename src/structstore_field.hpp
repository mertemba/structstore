#ifndef STRUCTSTORE_FIELD_HPP
#define STRUCTSTORE_FIELD_HPP

#include <iostream>
#include <type_traits>
#include <yaml-cpp/yaml.h>

#include "hashstring.hpp"
#include "serialization.hpp"

class StructStoreBase;

template<class Subclass>
class StructStore;

enum class FieldTypeValue {
    INT,
    STRING,
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
struct FieldType<bool> {
    static constexpr auto value = FieldTypeValue::BOOL;
};

template<>
struct FieldType<StructStoreBase> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

template<typename T>
struct FieldType<T, std::enable_if_t<std::is_base_of_v<StructStore<T>, T>>> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

class StructStoreField {
private:
    void* data;
    SerializeTextFunc* ser_text_func;
    SerializeYamlFunc* ser_yaml_func;

public:
    const FieldTypeValue type;

    template<typename T>
    StructStoreField(T& field)
            :data(&field),
             type(FieldType<T>::value),
             ser_text_func(serialize_text<T>),
             ser_yaml_func(serialize_yaml<T>) {}

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self) {
        self.ser_text_func(os, self.data);
        return os;
    }

    friend YAML::Node to_yaml(const StructStoreField& self) {
        return self.ser_yaml_func(self.data);
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

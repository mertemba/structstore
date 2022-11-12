#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <iostream>
#include <yaml-cpp/yaml.h>
#include "structstore_alloc.hpp"
#include "structstore_field.hpp"

namespace structstore {

std::ostream& operator<<(std::ostream&, const StructStore&);

template<typename T>
void serialize_text(std::ostream& os, void* val) {
    os << *(T*) val;
}

template<>
void serialize_text<bool>(std::ostream& os, void* val) {
    os << (*(bool*) val ? "true" : "false");
}

template<>
void serialize_text<structstore::string>(std::ostream& os, void* val) {
    os << '"' << *(structstore::string*) val << '"';
}

YAML::Node to_yaml(const int& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const double& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const bool& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const structstore::string& val) {
    return YAML::Node(val.c_str());
}

YAML::Node to_yaml(const StructStore&);

template<typename T>
YAML::Node serialize_yaml(void* val) {
    return to_yaml(*(T*) val);
}

using SerializeTextFunc = void(std::ostream&, void*);
using SerializeYamlFunc = YAML::Node(void*);

enum class FieldTypeValue : uint8_t;

void serialize_text(std::ostream& os, FieldTypeValue type, void* data) {
    switch (type) {
        case FieldTypeValue::INT:
            serialize_text<int>(os, data);
            break;
        case FieldTypeValue::DOUBLE:
            serialize_text<double>(os, data);
            break;
        case FieldTypeValue::STRING:
            serialize_text<structstore::string>(os, data);
            break;
        case FieldTypeValue::BOOL:
            serialize_text<bool>(os, data);
            break;
        case FieldTypeValue::STRUCT:
            serialize_text<StructStore>(os, data);
            break;
        default:
            throw std::runtime_error("internal error: unknown field type");
    }
}

YAML::Node serialize_yaml(FieldTypeValue type, void* data) {
    switch (type) {
        case FieldTypeValue::INT:
            return serialize_yaml<int>(data);
        case FieldTypeValue::DOUBLE:
            return serialize_yaml<double>(data);
        case FieldTypeValue::STRING:
            return serialize_yaml<structstore::string>(data);
        case FieldTypeValue::BOOL:
            return serialize_yaml<bool>(data);
        case FieldTypeValue::STRUCT:
            return serialize_yaml<StructStore>(data);
        default:
            throw std::runtime_error("internal error: unknown field type");
    }
}

}

#endif

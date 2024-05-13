#ifndef STST_SERIALIZATION_HPP
#define STST_SERIALIZATION_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"

#include <iostream>

#include <yaml-cpp/yaml.h>

namespace structstore {

std::ostream& operator<<(std::ostream&, const StructStore&);

std::ostream& operator<<(std::ostream&, const List&);

std::ostream& operator<<(std::ostream&, const Matrix&);

template<typename T>
void serialize_text(std::ostream& os, void* val) {
    os << *(T*) val;
}

template<>
void serialize_text<bool>(std::ostream& os, void* val);

template<>
void serialize_text<structstore::string>(std::ostream& os, void* val);

YAML::Node to_yaml(const int& val);

YAML::Node to_yaml(const double& val);

YAML::Node to_yaml(const bool& val) ;

YAML::Node to_yaml(const structstore::string& val);

YAML::Node to_yaml(const StructStore&);

YAML::Node to_yaml(const List&);

template<typename T>
YAML::Node serialize_yaml(void* val) {
    return to_yaml(*(T*) val);
}

using SerializeTextFunc = void(std::ostream&, void*);
using SerializeYamlFunc = YAML::Node(void*);

enum class FieldTypeValue : uint8_t;

void serialize_text(std::ostream& os, FieldTypeValue type, void* data);

YAML::Node serialize_yaml(FieldTypeValue type, void* data);

}

#endif

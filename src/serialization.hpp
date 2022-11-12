#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <iostream>
#include <yaml-cpp/yaml.h>
#include "structstore_alloc.hpp"

namespace structstore {

using SerializeTextFunc = void(std::ostream&, void*);
using SerializeYamlFunc = YAML::Node(void*);

enum class FieldTypeValue : uint8_t;
extern std::unordered_map<FieldTypeValue, SerializeTextFunc*> ser_text_funcs;
extern std::unordered_map<FieldTypeValue, SerializeYamlFunc*> ser_yaml_funcs;

YAML::Node to_yaml(const int& val);

YAML::Node to_yaml(const bool& val);

YAML::Node to_yaml(const structstore::string& val);

}

#endif

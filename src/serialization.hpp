#ifndef SERIALIZATION_HPP
#define SERIALIZATION_HPP

#include <iostream>
#include <yaml-cpp/yaml.h>

using SerializeTextFunc = void(std::ostream&, void*);
using SerializeYamlFunc = YAML::Node(void*);

template<typename T>
void serialize_text(std::ostream& os, void* val) {
    os << *(T*) val;
}

template<>
void serialize_text<bool>(std::ostream& os, void* val) {
    os << (*(bool*) val ? "true" : "false");
}

template<>
void serialize_text<std::string>(std::ostream& os, void* val) {
    os << '"' << *(std::string*) val << '"';
}

YAML::Node to_yaml(const int& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const bool& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const std::string& val) {
    return YAML::Node(val);
}

template<typename T>
YAML::Node serialize_yaml(void* val) {
    return to_yaml(*(T*) val);
}

#endif

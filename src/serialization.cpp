#include "serialization.hpp"
#include "structstore.hpp"

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

template<typename T>
YAML::Node serialize_yaml(void* val) {
    return to_yaml(*(T*) val);
}

std::unordered_map<FieldTypeValue, SerializeTextFunc*> ser_text_funcs{
        {FieldTypeValue::INT,       serialize_text<int>},
        {FieldTypeValue::STRING,    serialize_text<std::string>},
        {FieldTypeValue::ARENA_STR, serialize_text<arena_str>},
        {FieldTypeValue::BOOL,      serialize_text<bool>},
        {FieldTypeValue::STRUCT,    serialize_text<StructStoreBase>},
};

std::unordered_map<FieldTypeValue, SerializeYamlFunc*> ser_yaml_funcs{
        {FieldTypeValue::INT,       serialize_yaml<int>},
        {FieldTypeValue::STRING,    serialize_yaml<std::string>},
        {FieldTypeValue::ARENA_STR, serialize_yaml<arena_str>},
        {FieldTypeValue::BOOL,      serialize_yaml<bool>},
        {FieldTypeValue::STRUCT,    serialize_yaml<StructStoreBase>},
};

YAML::Node to_yaml(const int& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const bool& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const std::string& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const arena_str& val) {
    return YAML::Node(val.c_str());
}

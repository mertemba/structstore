#include "serialization.hpp"
#include "structstore.hpp"

namespace structstore {

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

template<typename T>
YAML::Node serialize_yaml(void* val) {
    return to_yaml(*(T*) val);
}

std::unordered_map<FieldTypeValue, SerializeTextFunc*> ser_text_funcs{
        {FieldTypeValue::INT,    serialize_text<int>},
        {FieldTypeValue::STRING, serialize_text<structstore::string>},
        {FieldTypeValue::BOOL,   serialize_text<bool>},
        {FieldTypeValue::STRUCT, serialize_text<StructStoreBase>},
};

std::unordered_map<FieldTypeValue, SerializeYamlFunc*> ser_yaml_funcs{
        {FieldTypeValue::INT,    serialize_yaml<int>},
        {FieldTypeValue::STRING, serialize_yaml<structstore::string>},
        {FieldTypeValue::BOOL,   serialize_yaml<bool>},
        {FieldTypeValue::STRUCT, serialize_yaml<StructStoreBase>},
};

YAML::Node to_yaml(const int& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const bool& val) {
    return YAML::Node(val);
}

YAML::Node to_yaml(const structstore::string& val) {
    return YAML::Node(val.c_str());
}

}

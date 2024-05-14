#include "structstore/stst_serialization.hpp"

using namespace structstore;

template<>
void structstore::serialize_text<bool>(std::ostream& os, void* val) {
    os << (*(bool*) val ? "true" : "false");
}

template<>
void structstore::serialize_text<structstore::string>(std::ostream& os, void* val) {
    os << '"' << *(structstore::string*) val << '"';
}

YAML::Node structstore::to_yaml(const int& val) {
    return YAML::Node(val);
}

YAML::Node structstore::to_yaml(const double& val) {
    return YAML::Node(val);
}

YAML::Node structstore::to_yaml(const bool& val) {
    return YAML::Node(val);
}

YAML::Node structstore::to_yaml(const structstore::string& val) {
    return YAML::Node(val.c_str());
}

void structstore::serialize_text(std::ostream& os, FieldTypeValue type, void* data) {
    switch (type) {
        case FieldTypeValue::EMPTY:
            os << "<empty>";
            break;
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
        case FieldTypeValue::LIST:
            serialize_text<List>(os, data);
            break;
        case FieldTypeValue::MATRIX:
            serialize_text<Matrix>(os, data);
            break;
        default:
            throw std::runtime_error("internal error: unknown field type");
    }
}

YAML::Node structstore::serialize_yaml(FieldTypeValue type, void* data) {
    switch (type) {
        case FieldTypeValue::EMPTY:
            return YAML::Node(YAML::Null);
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
        case FieldTypeValue::LIST:
            return serialize_yaml<List>(data);
        case FieldTypeValue::MATRIX:
            throw std::runtime_error("YAML serialization of matrices is not supported yet");
        default:
            throw std::runtime_error("internal error: unknown field type");
    }
}

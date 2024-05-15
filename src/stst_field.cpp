#include "structstore/stst_field.hpp"

using namespace structstore;

std::ostream& structstore::operator<<(std::ostream& os, const StructStoreField& field) {
    typing::SerializeTextFn<> serializer = typing::get_serializer_text(field.get_type());
    serializer(os, field.data);
    return os;
}

YAML::Node structstore::to_yaml(const StructStoreField& field) {
    typing::SerializeYamlFn<> serializer = typing::get_serializer_yaml(field.get_type());
    return serializer(field.data);
}

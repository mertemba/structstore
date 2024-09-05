#include "structstore/stst_field.hpp"

using namespace structstore;

template<>
std::ostream& structstore::to_text(std::ostream& os, const StructStoreField& field) {
    typing::SerializeTextFn<> serializer = typing::get_serializer_text(field.get_type_hash());
    serializer(os, field.data);
    return os;
}

template<>
YAML::Node structstore::to_yaml(const StructStoreField& field) {
    typing::SerializeYamlFn<> serializer = typing::get_serializer_yaml(field.get_type_hash());
    return serializer(field.data);
}

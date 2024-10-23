#include "structstore/stst_field.hpp"
#include "structstore/stst_shared.hpp"

using namespace structstore;

template<>
std::ostream& structstore::to_text(std::ostream& os, const StructStoreField& field) {
    const auto& field_type = typing::get_type(field.get_type_hash());
    field_type.serialize_text_fn(os, field.data);
    return os;
}

template<>
YAML::Node structstore::to_yaml(const StructStoreField& field) {
    const auto& field_type = typing::get_type(field.get_type_hash());
    return field_type.serialize_yaml_fn(field.data);
}

void StructStoreField::copy_from(MiniMalloc& mm_alloc, const StructStoreField& other) {
    assert_empty();
    type_hash = other.type_hash;
    const auto& field_type = typing::get_type(type_hash);
    data = mm_alloc.allocate(field_type.size);
    field_type.constructor_fn(mm_alloc, data);
    field_type.copy_fn(mm_alloc, data, other.data);
}

void StructStoreField::move_from(StructStoreField& other) {
    assert_empty();
    std::swap(type_hash, other.type_hash);
    std::swap(data, other.data);
}

template<>
FieldView::FieldView(StructStoreShared& store) : field{&store->get_store()} {}

#include "structstore/stst_field.hpp"
#include "structstore/stst_shared.hpp"

using namespace structstore;

void Field::to_text(std::ostream& os) const {
    const auto& field_type = typing::get_type(type_hash);
    field_type.serialize_text_fn(os, data);
}

YAML::Node Field::to_yaml() const {
    const auto& field_type = typing::get_type(type_hash);
    return field_type.serialize_yaml_fn(data);
}

void Field::copy_from(MiniMalloc& mm_alloc, const Field& other) {
    assert_empty();
    type_hash = other.type_hash;
    const auto& field_type = typing::get_type(type_hash);
    data = mm_alloc.allocate(field_type.size);
    field_type.constructor_fn(mm_alloc, data);
    field_type.copy_fn(mm_alloc, data, other.data);
}

void Field::move_from(Field& other) {
    assert_empty();
    std::swap(type_hash, other.type_hash);
    std::swap(data, other.data);
}

template<>
FieldView::FieldView(StructStoreShared& store) : field{&*store} {}

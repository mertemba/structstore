#include "structstore/stst_field.hpp"
#include "structstore/stst_shared.hpp"
#include <stdexcept>

using namespace structstore;

void Field::to_text(std::ostream& os) const {
    const auto& type_info = typing::get_type(type_hash);
    type_info.serialize_text_fn(os, data);
}

YAML::Node Field::to_yaml() const {
    const auto& type_info = typing::get_type(type_hash);
    return type_info.serialize_yaml_fn(data);
}

void Field::constr_copy_from(MiniMalloc& mm_alloc, const Field& other) {
    assert_empty();
    type_hash = other.type_hash;
    const auto& type_info = typing::get_type(type_hash);
    data = mm_alloc.allocate(type_info.size);
    type_info.constructor_fn(mm_alloc, data);
    type_info.copy_fn(mm_alloc, data, other.data);
}

void Field::copy_from(MiniMalloc& mm_alloc, const Field& other) {
    assert_nonempty();
    if (type_hash != other.type_hash) {
        throw std::runtime_error("copying field with different type");
    }
    const auto& type_info = typing::get_type(type_hash);
    type_info.copy_fn(mm_alloc, data, other.data);
}

void Field::move_from(Field& other) {
    assert_empty();
    std::swap(type_hash, other.type_hash);
    std::swap(data, other.data);
}

template<>
FieldView::FieldView(StructStoreShared& store) : field{&*store} {}

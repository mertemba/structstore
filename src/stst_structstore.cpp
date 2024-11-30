#include "structstore/stst_structstore.hpp"

using namespace structstore;

const TypeInfo& StructStore::type_info =
        typing::register_type<StructStore>("structstore::StructStore");

void StructStore::check() const {
    field_map.check();
    for (const auto& [key, value]: field_map.get_fields()) {
        try_with_info("in field '" << key.str << "' value: ",
                      value.check(field_map.get_alloc(), *this););
    }
}

void StructStore::check(const MiniMalloc& mm_alloc, const FieldTypeBase* parent_field) const {
    if (&mm_alloc != &field_map.get_alloc()) {
        throw std::runtime_error("internal error: allocators are not the same");
    }
    if (this->parent_field != parent_field) {
        throw std::runtime_error("invalid parent_field pointer in field of type " + type_info.name);
    }
    check();
}

FieldAccess<true> StructStore::at(HashString name) {
    return FieldAccess<true>{field_map.at(name), field_map.get_alloc(), this};
}

FieldAccess<true> StructStore::operator[](HashString name) {
    return FieldAccess<true>{field_map[name], field_map.get_alloc(), this};
}

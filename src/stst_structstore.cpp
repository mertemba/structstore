#include "structstore/stst_structstore.hpp"
#include "structstore/stst_callstack.hpp"

using namespace structstore;

const TypeInfo& StructStore::type_info =
        typing::register_type<StructStore>("structstore::StructStore");

void StructStore::check(const SharedAlloc* sh_alloc) const {
    CallstackEntry entry{"structstore::StructStore::check()"};
    field_map.check(sh_alloc, *this);
}

FieldAccess<true> StructStore::at(const std::string& name) {
    return FieldAccess<true>{field_map.at(name), field_map.get_alloc(), this};
}

FieldAccess<true> StructStore::operator[](const std::string& name) {
    return FieldAccess<true>{field_map[name], field_map.get_alloc(), this};
}

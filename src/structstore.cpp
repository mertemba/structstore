#include "structstore/stst_structstore.hpp"

using namespace structstore;

static constexpr size_t malloc_size = 1 << 16;
MiniMalloc structstore::static_alloc{malloc_size, std::malloc(malloc_size)};

std::ostream& structstore::operator<<(std::ostream& os, const StructStore& self) {
    os << "{";
    for (const auto& name: self.slots) {
        os << '"' << name.str << "\":" << self.fields.at(name) << ",";
    }
    os << "}";
    return os;
}

YAML::Node structstore::to_yaml(const StructStore& self) {
    YAML::Node root(YAML::NodeType::Map);
    for (const auto& name: self.slots) {
        root[name.str] = to_yaml(self.fields.at(name));
    }
    return root;
}

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value) {
    get<structstore::string>() = value;
    return *this;
}

template<>
FieldAccess& FieldAccess::operator=<std::string>(const std::string& value) {
    return *this = value.c_str();
}

void structstore::destruct(StructStore& store) {
    store.~StructStore();
}

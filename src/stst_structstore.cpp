#include "structstore/stst_structstore.hpp"

using namespace structstore;

static constexpr size_t malloc_size = 1 << 20;
MiniMalloc structstore::static_alloc(malloc_size, std::malloc(malloc_size));

void StructStore::register_type() {
    typing::register_type<StructStore>("structstore::StructStore");
    typing::register_mm_alloc_constructor<StructStore>();
    typing::register_default_destructor<StructStore>();
    typing::register_default_serializer_text<StructStore>();
    typing::register_serializer_yaml<StructStore>(
            [](const StructStore* store) {
                return to_yaml(*store);
            });
};

std::ostream& structstore::operator<<(std::ostream& os, const StructStore& store) {
    os << "{";
    for (const auto& name: store.slots) {
        os << '"' << name.str << "\":" << store.fields.at(name) << ",";
    }
    os << "}";
    return os;
}

YAML::Node structstore::to_yaml(const StructStore& store) {
    YAML::Node root(YAML::NodeType::Map);
    for (const auto& name: store.slots) {
        root[name.str] = to_yaml(store.fields.at(name));
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

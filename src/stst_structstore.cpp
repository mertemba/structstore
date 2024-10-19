#include "structstore/stst_structstore.hpp"

using namespace structstore;

static constexpr size_t malloc_size = 1 << 20;
MiniMalloc structstore::static_alloc(malloc_size, std::malloc(malloc_size));

template<>
std::ostream& structstore::to_text(std::ostream& os, const StructStore& store) {
    STST_LOG_DEBUG() << "serializing StructStore at " << &store;
    os << "{";
    for (const auto& name: store.slots) {
        STST_LOG_DEBUG() << "field " << (void*) &name << " is at " << &store.fields.at(name);
        os << '"' << name.str << "\":";
        to_text(os, store.fields.at(name)) << ",";
    }
    os << "}";
    return os;
}

template<>
YAML::Node structstore::to_yaml(const StructStore& store) {
    YAML::Node root(YAML::NodeType::Map);
    for (const auto& name: store.slots) {
        root[name.str] = to_yaml(store.fields.at(name));
    }
    return root;
}

void StructStore::register_type() {
    typing::register_type(typing::FieldType<StructStore>{
            .name = "structstore::StructStore",
            .constructor_fn = typing::mm_alloc_constructor_fn<StructStore>,
            .check_fn = [](MiniMalloc& mm_alloc, const StructStore* store) {
                try_with_info("StructStore: ", mm_alloc.assert_owned(store););
                try_with_info("StructStore content: ", store->check(););
            }});
};

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value) {
    get<structstore::string>() = value;
    return *this;
}

template<>
FieldAccess& FieldAccess::operator=<std::string>(const std::string& value) {
    return *this = value.c_str();
}

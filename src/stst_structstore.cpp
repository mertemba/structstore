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
    typing::register_type<StructStore>("structstore::StructStore");
    typing::register_mm_alloc_constructor<StructStore>();
    typing::register_default_destructor<StructStore>();
    typing::register_default_serializer_text<StructStore>();
    typing::register_default_serializer_yaml<StructStore>();
    typing::register_check<StructStore>([](MiniMalloc& mm_alloc, const StructStore* store) {
        try_with_info("StructStore: ", mm_alloc.assert_owned(store););
        try_with_info("StructStore content: ", store->check(););
    });
    typing::register_default_cmp_equal_fn<StructStore>();
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

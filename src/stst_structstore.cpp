#include "structstore/stst_structstore.hpp"
#include "structstore/stst_containers.hpp"

using namespace structstore;

static constexpr size_t malloc_size = 1 << 20;
MiniMalloc structstore::static_alloc(malloc_size, std::malloc(malloc_size));

void StructStore::to_text(std::ostream& os) const {
    STST_LOG_DEBUG() << "serializing StructStore at " << this;
    os << "{";
    for (const auto& name: slots) {
        STST_LOG_DEBUG() << "field " << (void*) &name << " is at " << &fields.at(name);
        os << '"' << name.str << "\":";
        os << fields.at(name) << ",";
    }
    os << "}";
}

YAML::Node StructStore::to_yaml() const {
    YAML::Node root(YAML::NodeType::Map);
    for (const auto& name: slots) {
        root[name.str] = fields.at(name).to_yaml();
    }
    return root;
}

void StructStore::check(MiniMalloc& mm_alloc) const {
    if (&mm_alloc != &this->mm_alloc) {
        throw std::runtime_error("internal error: allocators are not the same");
    }
    if (slots.size() != fields.size()) {
        throw std::runtime_error("internal error: slots and fields with different size");
    }
    for (const HashString& str: slots) {
        try_with_info("in slot '" << str.str << "' name: ", mm_alloc.assert_owned(str.str););
    }
    for (const auto& [key, value]: fields) {
        try_with_info("in field '" << key.str << "' name: ", mm_alloc.assert_owned(key.str););
        if (managed) {
            try_with_info("in field '" << key.str << "' value: ", value.check(mm_alloc););
        }
    }
}

void StructStore::register_type() {
    typing::register_type<StructStore>("structstore::StructStore");
}

::structstore::String& FieldAccess::get_str() { return get<::structstore::String>(); }

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value) {
    get<structstore::String>() = value;
    return *this;
}

template<>
FieldAccess& FieldAccess::operator=<std::string>(const std::string& value) {
    return *this = value.c_str();
}

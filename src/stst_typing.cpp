#include "structstore/stst_typing.hpp"
#include "structstore/stst_containers.hpp"

using namespace structstore;

std::unordered_map<std::type_index, uint64_t>& typing::get_type_hashes() {
    static auto* types = new std::unordered_map<std::type_index, uint64_t>();
    return *types;
}

std::unordered_map<uint64_t, const typing::FieldType<>>& typing::get_field_types() {
    static auto* field_types = new std::unordered_map<uint64_t, const typing::FieldType<>>();
    return *field_types;
}

const typing::FieldType<void>& typing::get_field_type(uint64_t type_hash) {
    try {
        return get_field_types().at(type_hash);
    } catch (const std::out_of_range&) {
        std::ostringstream str;
        str << "could not find type information for type hash " << type_hash;
        throw std::runtime_error(str.str());
    }
}

template<>
uint64_t typing::get_type_hash<void>() {
    return 0;
}

static bool register_common_types_() {
    // empty
    typing::register_type(typing::FieldType<void>{
            .name = "<empty>",
            .constructor_fn = [](MiniMalloc&, void*) {},
            .destructor_fn = [](MiniMalloc&, void*) {},
            .serialize_text_fn = [](std::ostream& os, const void*) -> std::ostream& {
                return os << "<empty>";
            },
            .serialize_yaml_fn = [](const void*) { return YAML::Node(YAML::Null); },
            .cmp_equal_fn = [](const void*, const void*) { return true; }});

    typing::register_type(typing::FieldType<int>{.name = "int"});
    typing::register_type(typing::FieldType<double>{.name = "double"});
    typing::register_type(typing::FieldType<bool>{.name = "bool"});

    typing::register_type(typing::FieldType<structstore::string>{
            .name = "structstore::string",
            .constructor_fn = typing::stl_alloc_constructor_fn<structstore::string>,
            .check_fn = [](MiniMalloc& mm_alloc, const structstore::string* str) {
                try_with_info("string: ", mm_alloc.assert_owned(str););
                try_with_info("string data: ", mm_alloc.assert_owned(str->data()););
            }});

    return true;
}

void typing::register_common_types() {
    static bool success = register_common_types_();
    if (success) {
        List::register_type();
        Matrix::register_type();
        StructStore::register_type();
    }
    success = false;
}

static bool registered_common_types = []() {
    typing::register_common_types();
    return true;
}();

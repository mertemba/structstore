#include "structstore/stst_typing.hpp"
#include "structstore/stst_containers.hpp"

using namespace structstore;

std::unordered_map<std::type_index, uint64_t>& typing::get_type_hashes() {
    static auto* types = new std::unordered_map<std::type_index, uint64_t>();
    return *types;
}

std::unordered_map<uint64_t, std::string>& typing::get_type_names() {
    static auto* type_names = new std::unordered_map<uint64_t, std::string>();
    return *type_names;
}

std::unordered_map<uint64_t, typing::ConstructorFn<>>& typing::get_constructors() {
    static auto* constructors = new std::unordered_map<uint64_t, typing::ConstructorFn<>>();
    return *constructors;
}

std::unordered_map<uint64_t, typing::DestructorFn<>>& typing::get_destructors() {
    static auto* destructors = new std::unordered_map<uint64_t, typing::DestructorFn<>>();
    return *destructors;
}

std::unordered_map<uint64_t, typing::SerializeTextFn<>>& typing::get_serializers_text() {
    static auto* serializers_text = new std::unordered_map<uint64_t, typing::SerializeTextFn<>>();
    return *serializers_text;
}

std::unordered_map<uint64_t, typing::SerializeYamlFn<>>& typing::get_serializers_yaml() {
    static auto* serializers_yaml = new std::unordered_map<uint64_t, typing::SerializeYamlFn<>>();
    return *serializers_yaml;
}

std::unordered_map<uint64_t, typing::CheckFn<>>& typing::get_checks() {
    static auto* checks = new std::unordered_map<uint64_t, typing::CheckFn<>>();
    return *checks;
}

std::unordered_map<uint64_t, typing::CmpEqualFn<>>& typing::get_cmp_equal_fns() {
    static auto* cmp_equal_fns = new std::unordered_map<uint64_t, typing::CmpEqualFn<>>();
    return *cmp_equal_fns;
}

template<>
uint64_t typing::get_type_hash<void>() {
    return 0;
}

static bool register_common_types_() {
    // empty
    typing::register_type<void>("<empty>");
    typing::register_serializer_text<void>([](std::ostream& os, const void*) -> std::ostream& {
        return os << "<empty>";
    });
    typing::register_serializer_yaml<void>([](const void*) {
        return YAML::Node(YAML::Null);
    });

    typing::register_basic_type<int>("int");
    typing::register_basic_type<double>("double");
    typing::register_basic_type<bool>("bool");

    typing::register_type<structstore::string>("structstore::string");
    typing::register_stl_alloc_constructor<structstore::string>();
    typing::register_default_destructor<structstore::string>();
    typing::register_default_serializer_text<structstore::string>();
    typing::register_default_serializer_yaml<structstore::string>();
    typing::register_check<structstore::string>([](MiniMalloc& mm_alloc, const structstore::string* str) {
        try_with_info("string: ", mm_alloc.assert_owned(str););
        try_with_info("string data: ", mm_alloc.assert_owned(str->data()););
    });
    typing::register_default_cmp_equal_fn<structstore::string>();

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

#include "structstore/stst_typing.hpp"
#include "structstore/stst_field.hpp"

using namespace structstore;

std::unordered_map<std::type_index, typing::ConstructorFn<>>& typing::get_constructors() {
    static auto* constructors = new std::unordered_map<std::type_index, typing::ConstructorFn<>>();
    return *constructors;
}

std::unordered_map<std::type_index, typing::DestructorFn<>>& typing::get_destructors() {
    static auto* destructors = new std::unordered_map<std::type_index, typing::DestructorFn<>>();
    return *destructors;
}

std::unordered_map<std::type_index, typing::SerializeTextFn<>>& typing::get_serializers_text() {
    static auto* serializers_text = new std::unordered_map<std::type_index, typing::SerializeTextFn<>>();
    return *serializers_text;
}

std::unordered_map<std::type_index, typing::SerializeYamlFn<>>& typing::get_serializers_yaml() {
    static auto* serializers_yaml = new std::unordered_map<std::type_index, typing::SerializeYamlFn<>>();
    return *serializers_yaml;
}

template<typename T>
static void register_basic_type() {
    typing::register_default_constructor<T>();
    typing::register_default_destructor<T>();
    typing::register_default_serializer_text<T>();
    typing::register_default_serializer_yaml<T>();
}

bool typing::registered_common_types = []() {
    register_basic_type<int>();
    register_basic_type<double>();
    register_basic_type<bool>();

    typing::register_stl_alloc_constructor<structstore::string>();
    typing::register_default_destructor<structstore::string>();
    typing::register_default_serializer_text<structstore::string>();
    typing::register_serializer_yaml<structstore::string>([](const structstore::string* str) {
        return YAML::Node(str->c_str());
    });

    return true;
}();

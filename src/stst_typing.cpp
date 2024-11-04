#include "structstore/stst_typing.hpp"

using namespace structstore;

std::unordered_map<std::type_index, uint64_t>& typing::get_type_hashes() {
    static auto* types = new std::unordered_map<std::type_index, uint64_t>();
    return *types;
}

std::unordered_map<uint64_t, const FieldType>& typing::get_field_types() {
    static auto* field_types = new std::unordered_map<uint64_t, const FieldType>();
    return *field_types;
}

const FieldType& typing::get_type(uint64_t type_hash) {
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

static bool registered_common_types = []() {
    typing::register_type<void>("<empty>");
    typing::register_type<int>("int");
    typing::register_type<double>("double");
    typing::register_type<bool>("bool");
    return true;
}();

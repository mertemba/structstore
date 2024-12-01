#include "structstore/stst_typing.hpp"

using namespace structstore;

void FieldTypeBase::read_lock_() const {
    if (parent_field) { parent_field->read_lock_(); }
    mutex.read_lock();
}

void FieldTypeBase::read_unlock_() const {
    mutex.read_unlock();
    if (parent_field) { parent_field->read_unlock_(); }
}

void FieldTypeBase::write_lock_() const {
    // the read lock here is intentional
    if (parent_field) { parent_field->read_lock_(); }
    mutex.write_lock();
}

void FieldTypeBase::write_unlock_() const {
    // the read lock here is intentional
    mutex.write_unlock();
    if (parent_field) { parent_field->read_unlock_(); }
}

std::unordered_map<std::type_index, uint64_t>& typing::get_type_hashes() {
    static auto* types = new std::unordered_map<std::type_index, uint64_t>();
    return *types;
}

std::unordered_map<uint64_t, const TypeInfo>& typing::get_type_infos() {
    static auto* type_infos = new std::unordered_map<uint64_t, const TypeInfo>();
    return *type_infos;
}

const TypeInfo& typing::get_type(uint64_t type_hash) {
    try {
        return get_type_infos().at(type_hash);
    } catch (const std::out_of_range&) {
        std::ostringstream str;
        str << "could not find type information for type hash " << type_hash;
        throw std::runtime_error(str.str());
    }
}

static bool registered_common_types = []() {
    typing::register_type<void>("<empty>");
    typing::register_type<int>("int");
    typing::register_type<double>("double");
    typing::register_type<bool>("bool");
    return true;
}();

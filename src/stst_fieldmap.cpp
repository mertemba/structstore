#include "structstore/stst_fieldmap.hpp"

using namespace structstore;

HashString FieldMapBase::internalize_string(const HashString& str) {
    size_t len = std::strlen(str.str);
    char* buf = (char*) mm_alloc.allocate(len + 1);
    std::strcpy(buf, str.str);
    return {buf, str.hash};
}

bool FieldMapBase::operator==(const FieldMapBase& other) const {
    return slots == other.slots && fields == other.fields;
}

Field* FieldMapBase::try_get_field(HashString name) {
    auto it = fields.find(name);
    if (it == fields.end()) { return nullptr; }
    return &it->second;
}

const Field* FieldMapBase::try_get_field(HashString name) const {
    auto it = fields.find(name);
    if (it == fields.end()) { return nullptr; }
    return &it->second;
}

void FieldMapBase::to_text(std::ostream& os) const {
    STST_LOG_DEBUG() << "serializing StructStore at " << this;
    os << "{";
    for (const auto& name: slots) {
        STST_LOG_DEBUG() << "field " << &name << " is at " << &fields.at(name);
        os << '"' << name.str << "\":";
        fields.at(name).to_text(os);
        os << ",";
    }
    os << "}";
}

YAML::Node FieldMapBase::to_yaml() const {
    YAML::Node root(YAML::NodeType::Map);
    for (const auto& name: slots) { root[name.str] = fields.at(name).to_yaml(); }
    return root;
}

void FieldMapBase::check() const {
    if (slots.size() != fields.size()) {
        throw std::runtime_error("internal error: slots and fields with different size");
    }
    for (const HashString& str: slots) {
        try_with_info("in slot '" << str.str << "' name: ", mm_alloc.assert_owned(str.str););
    }
    for (const auto& [key, value]: fields) {
        try_with_info("in field '" << key.str << "' name: ", mm_alloc.assert_owned(key.str););
    }
}

template<>
void FieldMap<false>::copy_from_unmanaged(const FieldMap<false>& other) {
    // unmanaged copy: slots have to be the same
    STST_LOG_DEBUG() << "copying FieldMap from " << &other << " into " << this;
    if (slots != other.get_slots()) {
        throw std::runtime_error("copying into unmanaged FieldMap with different fields");
    }
    for (const HashString& str: slots) {
        fields.at(str).copy_from(mm_alloc, other.get_fields().at(str));
    }
}

template<>
void FieldMap<true>::copy_from_managed(const FieldMap<true>& other,
                                       const FieldTypeBase* parent_field) {
    // managed copy: clear and insert the other contents
    STST_LOG_DEBUG() << "copying FieldMap from " << &other << " into " << this;
    clear();
    for (const HashString& str: other.get_slots()) {
        HashString name_int = internalize_string(str);
        slots.emplace_back(name_int);
        Field& field = fields.emplace(name_int, Field{}).first->second;
        field.construct_copy_from(mm_alloc, other.get_fields().at(str), parent_field);
    }
}

template<>
Field& FieldMap<true>::get_or_insert(HashString name) {
    auto it = fields.find(name);
    if (it == fields.end()) {
        HashString name_int = internalize_string(name);
        it = fields.emplace(name_int, Field{}).first;
        slots.emplace_back(name_int);
    }
    return it->second;
}

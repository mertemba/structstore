#include "structstore/stst_fieldmap.hpp"
#include "structstore/stst_alloc.hpp"
#include "structstore/stst_callstack.hpp"

using namespace structstore;

bool FieldMapBase::equal_slots(const FieldMapBase& other) const {
    if (slots.size() != other.slots.size()) return false;
    for (auto it1 = slots.begin(), it2 = other.slots.begin(); it1 != slots.end(); ++it1, ++it2) {
        if (**it1 != **it2) { return false; }
    }
    return true;
}

bool FieldMapBase::operator==(const FieldMapBase& other) const {
    if (slots.size() != other.slots.size()) return false;
    for (auto it1 = slots.begin(), it2 = other.slots.begin(); it1 != slots.end(); ++it1, ++it2) {
        if (**it1 != **it2) { return false; }
        if (fields.at(*it1) != other.fields.at(*it2)) { return false; }
    }
    return true;
}

Field* FieldMapBase::try_get_field(const std::string& name) {
    const shr_string* name_ = sh_alloc->strings().get(name);
    auto it = fields.find(name_);
    if (it == fields.end()) { return nullptr; }
    return &it->second;
}

const Field* FieldMapBase::try_get_field(const std::string& name) const {
    const shr_string* name_ = sh_alloc->strings().get(name);
    auto it = fields.find(name_);
    if (it == fields.end()) { return nullptr; }
    return &it->second;
}

void FieldMapBase::to_text(std::ostream& os) const {
    STST_LOG_DEBUG() << "serializing StructStore at " << this;
    os << "{";
    for (shr_string_ptr name: slots) {
        STST_LOG_DEBUG() << "field " << *name << " is at " << &fields.at(name);
        os << '"' << *name << "\":";
        fields.at(name).to_text(os);
        os << ",";
    }
    os << "}";
}

YAML::Node FieldMapBase::to_yaml() const {
    YAML::Node root(YAML::NodeType::Map);
    for (shr_string_ptr name: slots) { root[name->c_str()] = fields.at(name).to_yaml(); }
    return root;
}

void FieldMapBase::check(const SharedAlloc* sh_alloc, const FieldTypeBase& parent_field) const {
    CallstackEntry entry{"structstore::FieldMap::check()"};
    if (sh_alloc) {
        stst_assert(this->sh_alloc.get() == sh_alloc);
    } else {
        // use our own reference instead
        sh_alloc = this->sh_alloc.get();
    }
    // this could be allocated on regular stack/heap if the owning StructStore is not in shared mem
    stst_assert(sh_alloc == &static_alloc || sh_alloc->is_owned(this));
    if (slots.size() != fields.size()) {
        throw std::runtime_error("in FieldMap: slots and fields with different size");
    }
    for (shr_string_ptr str: slots) { stst_assert(sh_alloc->is_owned(str.get())); }
    for (const auto& [key, value]: fields) {
        stst_assert(sh_alloc->is_owned(key.get()));
        value.check(*sh_alloc, parent_field);
    }
}

template<>
void FieldMap<false>::copy_from_unmanaged(const FieldMap<false>& other) {
    // unmanaged copy: slots have to be the same
    STST_LOG_DEBUG() << "copying FieldMap from " << &other << " into " << this;
    if (!equal_slots(other)) {
        throw std::runtime_error("copying into unmanaged FieldMap with different field names");
    }
    for (auto it1 = get_slots().begin(), it2 = other.slots.begin(); it1 != slots.end();
         ++it1, ++it2) {
        fields.at(*it1).copy_from(*sh_alloc, other.fields.at(*it2));
    }
}

template<>
void FieldMap<true>::copy_from_managed(const FieldMap<true>& other,
                                       const FieldTypeBase* parent_field) {
    // managed copy: clear and insert the other contents
    STST_LOG_DEBUG() << "copying FieldMap from " << &other << " into " << this;
    clear();
    for (shr_string_ptr str: other.get_slots()) {
        const shr_string* name_int = sh_alloc->strings().internalize(std::string(*str));
        slots.emplace_back(name_int);
        Field& field = fields.emplace(name_int, Field{}).first->second;
        field.construct_copy_from(*sh_alloc, other.fields.at(str), parent_field);
    }
}

template<>
Field& FieldMap<true>::get_or_insert(const std::string& name) {
    auto it = fields.find(sh_alloc->strings().get(name));
    if (it == fields.end()) {
        const shr_string* name_int = sh_alloc->strings().internalize(name);
        it = fields.emplace(name_int, Field{}).first;
        slots.emplace_back(name_int);
    }
    return it->second;
}

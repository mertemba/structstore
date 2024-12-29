#include "structstore/stst_containers.hpp"
#include "structstore/stst_alloc.hpp"
#include "structstore/stst_callstack.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

using namespace structstore;

const TypeInfo& String::type_info = typing::register_type<String>("structstore::String");

void String::check(const MiniMalloc* mm_alloc) const {
    CallstackEntry entry{"structstore::String::check()"};
    if (mm_alloc && !empty()) { stst_assert(mm_alloc->is_owned(data())); }
}

String& String::operator=(const std::string& value) {
    static_cast<shr_string&>(*this) = value;
    return *this;
}

const TypeInfo& List::type_info = typing::register_type<List>("structstore::List");

void List::to_text(std::ostream& os) const {
    STST_LOG_DEBUG() << "serializing list at " << this;
    os << "[";
    for (const Field& field: data) {
        field.to_text(os);
        os << ",";
    }
    os << "]";
}

YAML::Node List::to_yaml() const {
    auto node = YAML::Node(YAML::NodeType::Sequence);
    for (const Field& field: data) {
        node.push_back(field.to_yaml());
    }
    return node;
}

void List::check(const MiniMalloc* mm_alloc) const {
    CallstackEntry entry{"structstore::List::check()"};
    if (mm_alloc) {
        stst_assert(&this->mm_alloc == mm_alloc);
    } else {
        // use our own reference instead
        mm_alloc = &this->mm_alloc;
    }
    for (const Field& field: data) { field.check(*mm_alloc, *this); }
}

bool List::operator==(const List& other) const {
    return data == other.data;
}

const TypeInfo& Matrix::type_info = typing::register_type<Matrix>("structstore::Matrix");

void Matrix::to_text(std::ostream& os) const {
    size_t size = 1;
    for (size_t i = 0; i < _ndim; ++i) {
        size *= _shape[i];
    }
    os << "[";
    for (size_t i = 0; i < size; ++i) {
        os << _data[i] << ",";
    }
    os << "]";
}

YAML::Node Matrix::to_yaml() const {
    throw std::runtime_error("serialize_yaml_fn not implemented for structstore::Matrix");
}

void Matrix::check(const MiniMalloc* mm_alloc) const {
    CallstackEntry entry{"structstore::Matrix::check()"};
    if (mm_alloc) {
        stst_assert(&this->mm_alloc == mm_alloc);
    } else {
        // use our own reference instead
        mm_alloc = &this->mm_alloc;
    }
    if (_data) { stst_assert(mm_alloc->is_owned(_data)); }
}

bool Matrix::operator==(const Matrix& other) const {
    if (_ndim != other._ndim) {
        return false;
    }
    size_t size = 1;
    for (size_t i = 0; i < _ndim; ++i) {
        if (_shape[i] != other._shape[i]) {
            return false;
        }
        size *= _shape[i];
    }
    for (size_t i = 0; i < size; ++i) {
        if (_data[i] != other._data[i]) {
            return false;
        }
    }
    return true;
}

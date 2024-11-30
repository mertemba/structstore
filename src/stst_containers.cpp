#include "structstore/stst_containers.hpp"
#include "structstore/stst_alloc.hpp"
#include "structstore/stst_utils.hpp"

using namespace structstore;

const TypeInfo& String::type_info = typing::register_type<String>("structstore::String");

void String::check(const MiniMalloc& mm_alloc, const FieldTypeBase* parent_field) const {
    if (this->parent_field != parent_field) {
        throw std::runtime_error("invalid parent_field pointer in field of type " + type_info.name);
    }
    try_with_info("string: ", mm_alloc.assert_owned(this););
    if (!empty()) { try_with_info("string data: ", mm_alloc.assert_owned(data());); }
}

String& String::operator=(const char* const& value) {
    static_cast<std_string&>(*this) = value;
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

void List::check(const MiniMalloc& mm_alloc, const FieldTypeBase* parent_field) const {
    if (this->parent_field != parent_field) {
        throw std::runtime_error("invalid parent_field pointer in field of type " + type_info.name);
    }
    try_with_info("List*: ", mm_alloc.assert_owned(this););
    for (const Field& field: data) {
        try_with_info("in List iter: ", field.check(mm_alloc, *this););
    }
}

template<>
void List::push_back<const char*>(const char* const& value) {
    push_back().get<structstore::String>() = value;
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

void Matrix::check(const MiniMalloc& mm_alloc, const FieldTypeBase* parent_field) const {
    if (this->parent_field != parent_field) {
        throw std::runtime_error("invalid parent_field pointer in field of type " + type_info.name);
    }
    try_with_info("Matrix*: ", mm_alloc.assert_owned(this););
    if (_data) {
        try_with_info("Matrix data: ", mm_alloc.assert_owned(_data););
    }
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

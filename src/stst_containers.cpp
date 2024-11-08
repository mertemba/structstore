#include "structstore/stst_containers.hpp"
#include "structstore/stst_alloc.hpp"

using namespace structstore;

const TypeInfo& String::type_info = typing::register_type<String>("structstore::String");

const TypeInfo& List::type_info = typing::register_type<List>("structstore::List");

void List::to_text(std::ostream& os) const {
    STST_LOG_DEBUG() << "serializing list at " << this;
    os << "[";
    for (const Field& field: data) {
        os << field << ",";
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

void List::check(MiniMalloc& mm_alloc) const {
    try_with_info("List*: ", mm_alloc.assert_owned(this););
    for (const Field& field: data) {
        try_with_info("in List iter: ", field.check(mm_alloc););
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

void Matrix::check(MiniMalloc& mm_alloc) const {
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

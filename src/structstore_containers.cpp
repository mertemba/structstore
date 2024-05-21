#include "structstore/stst_containers.hpp"

using namespace structstore;

std::ostream& structstore::operator<<(std::ostream& os, const List& self) {
    os << "[";
    for (const StructStoreField& field: self.data) {
        os << field << ",";
    }
    return os << "]";
}

template<>
void List::push_back<const char*>(const char* const& value) {
    push_back().get<structstore::string>() = value;
}

YAML::Node structstore::to_yaml(const List& list) {
    auto node = YAML::Node(YAML::NodeType::Sequence);
    for (const StructStoreField& field: list) {
        node.push_back(to_yaml(field));
    }
    return node;
}

std::ostream& structstore::operator<<(std::ostream& os, const Matrix& matrix) {
    size_t size = 1;
    for (size_t i = 0; i < matrix._ndim; ++i) {
        size *= matrix._shape[i];
    }
    os << "[";
    for (size_t i = 0; i < size; ++i) {
        os << matrix._data[i] << ",";
    }
    return os << "]";
}

void structstore::destruct(List& list) {
    list.~List();
}

void structstore::destruct(Matrix& matrix) {
    matrix.~Matrix();
}

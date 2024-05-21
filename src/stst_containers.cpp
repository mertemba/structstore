#include "structstore/stst_containers.hpp"

using namespace structstore;

void List::register_type() {
    typing::register_type<List>("structstore::List");
    typing::register_mm_alloc_constructor<List>();
    typing::register_default_destructor<List>();
    typing::register_default_serializer_text<List>();
    typing::register_serializer_yaml<List>(
            [](const List* list) -> YAML::Node {
                return to_yaml(*list);
            });
};

std::ostream& structstore::operator<<(std::ostream& os, const List& list) {
    os << "[";
    for (const StructStoreField& field: list.data) {
        os << field << ",";
    }
    return os << "]";
}

YAML::Node structstore::to_yaml(const List& list) {
    auto node = YAML::Node(YAML::NodeType::Sequence);
    for (const StructStoreField& field: list) {
        node.push_back(to_yaml(field));
    }
    return node;
}

template<>
void List::push_back<const char*>(const char* const& value) {
    push_back().get<structstore::string>() = value;
}

void Matrix::register_type() {
    typing::register_type<Matrix>("structstore::Matrix");
    typing::register_mm_alloc_constructor<Matrix>();
    typing::register_default_destructor<Matrix>();
    typing::register_default_serializer_text<Matrix>();
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

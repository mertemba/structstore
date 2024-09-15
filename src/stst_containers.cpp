#include "structstore/stst_containers.hpp"
#include "structstore/stst_alloc.hpp"

using namespace structstore;

void List::register_type() {
    typing::register_type<List>("structstore::List");
    typing::register_mm_alloc_constructor<List>();
    typing::register_default_destructor<List>();
    typing::register_default_serializer_text<List>();
    typing::register_default_serializer_yaml<List>();
    typing::register_check<List>([](MiniMalloc& mm_alloc, const List* list) {
        try_with_info("List*: ", mm_alloc.assert_owned(list););
        for (const StructStoreField& field: list->data) {
            try_with_info("in List iter: ", field.check(mm_alloc););
        }
    });
}

template<>
std::ostream& structstore::to_text(std::ostream& os, const List& list) {
    os << "[";
    for (const StructStoreField& field: list.data) {
        to_text(os, field) << ",";
    }
    return os << "]";
}

template<>
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
    typing::register_check<Matrix>([](MiniMalloc& mm_alloc, const Matrix* matrix) {
        try_with_info("Matrix*: ", mm_alloc.assert_owned(matrix););
        if (matrix->_data) {
            try_with_info("Matrix data: ", mm_alloc.assert_owned(matrix->_data););
        }
    });
}

template<>
std::ostream& structstore::to_text(std::ostream& os, const Matrix& matrix) {
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

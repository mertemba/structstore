#include "structstore/stst_containers.hpp"

using namespace structstore;

bool List::has_constructor = typing::register_mm_alloc_constructor<List>();

bool List::has_destructor = typing::register_default_destructor<List>();

bool List::has_serializer_text = typing::register_serializer_text<List>(
        [](std::ostream& os, const List* list) -> std::ostream& {
            os << "[";
            for (const StructStoreField& field: list->data) {
                os << field << ",";
            }
            return os << "]";
        });

bool List::has_serializer_yaml = typing::register_serializer_yaml<List>(
        [](const List* list) -> YAML::Node {
            auto node = YAML::Node(YAML::NodeType::Sequence);
            for (const StructStoreField& field: *list) {
                node.push_back(to_yaml(field));
            }
            return node;
        });

template<>
void List::push_back<const char*>(const char* const& value) {
    push_back().get<structstore::string>() = value;
}

bool Matrix::has_constructor = typing::register_mm_alloc_constructor<Matrix>();

bool Matrix::has_destructor = typing::register_default_destructor<Matrix>();

bool Matrix::has_serializer_text = typing::register_serializer_text<Matrix>(
        [](std::ostream& os, const Matrix* matrix) -> std::ostream& {
            size_t size = 1;
            for (size_t i = 0; i < matrix->_ndim; ++i) {
                size *= matrix->_shape[i];
            }
            os << "[";
            for (size_t i = 0; i < size; ++i) {
                os << matrix->_data[i] << ",";
            }
            return os << "]";
        });

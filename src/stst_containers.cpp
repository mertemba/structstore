#include "structstore/stst_containers.hpp"

using namespace structstore;

bool List::registered_type = []() {
    typing::register_type<List>("structstore::List");
    typing::register_mm_alloc_constructor<List>();
    typing::register_default_destructor<List>();
    typing::register_serializer_text<List>(
            [](std::ostream& os, const List* list) -> std::ostream& {
                os << "[";
                for (const StructStoreField& field: list->data) {
                    os << field << ",";
                }
                return os << "]";
            });
    typing::register_serializer_yaml<List>(
            [](const List* list) -> YAML::Node {
                auto node = YAML::Node(YAML::NodeType::Sequence);
                for (const StructStoreField& field: *list) {
                    node.push_back(to_yaml(field));
                }
                return node;
            });
    return true;
}();

template<>
void List::push_back<const char*>(const char* const& value) {
    push_back().get<structstore::string>() = value;
}

bool Matrix::registered_type = []() {
    typing::register_type<Matrix>("structstore::Matrix");
    typing::register_mm_alloc_constructor<Matrix>();
    typing::register_default_destructor<Matrix>();
    typing::register_serializer_text<Matrix>(
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
    return true;
}();

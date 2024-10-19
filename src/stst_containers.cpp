#include "structstore/stst_containers.hpp"
#include "structstore/stst_alloc.hpp"

using namespace structstore;

void List::register_type() {
    typing::register_type(typing::FieldType<List>{
            .name = "structstore::List",
            .constructor_fn = typing::mm_alloc_constructor_fn<List>,
            .check_fn = [](MiniMalloc& mm_alloc, const List* list) {
                try_with_info("List*: ", mm_alloc.assert_owned(list););
                for (const StructStoreField& field: list->data) {
                    try_with_info("in List iter: ", field.check(mm_alloc););
                }
            }});
}

template<>
std::ostream& structstore::to_text(std::ostream& os, const List& list) {
    STST_LOG_DEBUG() << "serializing list at " << &list;
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

bool List::operator==(const List& other) const {
    return data == other.data;
}

void Matrix::register_type() {
    typing::register_type(typing::FieldType<Matrix>{
            .name = "structstore::Matrix",
            .constructor_fn = typing::mm_alloc_constructor_fn<Matrix>,
            .serialize_yaml_fn = [](const Matrix*) -> YAML::Node {
                throw std::runtime_error("serialize_yaml_fn not implemented for structstore::Matrix");
            },
            .check_fn = [](MiniMalloc& mm_alloc, const Matrix* matrix) {
                try_with_info("Matrix*: ", mm_alloc.assert_owned(matrix););
                if (matrix->_data) {
                    try_with_info("Matrix data: ", mm_alloc.assert_owned(matrix->_data););
                } }});
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

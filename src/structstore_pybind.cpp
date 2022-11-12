#include "structstore_pybind.hpp"

namespace structstore {

pybind11::object field_to_object(StructStoreField& field) {
    switch (field.type) {
        case FieldTypeValue::INT:
            return pybind11::int_((int&) field);
        case FieldTypeValue::STRING:
            return pybind11::str(((structstore::string&) field).c_str());
        case FieldTypeValue::BOOL:
            return pybind11::bool_((bool&) field);
        case FieldTypeValue::STRUCT: {
            auto& store = (StructStoreBase&) field;
            if (store.is_dynamic()) {
                return pybind11::cast((StructStoreDyn<0>&) store, pybind11::return_value_policy::reference);
            } else {
                return pybind11::cast(store, pybind11::return_value_policy::reference);
            }
        }
        default:
            std::cerr << "internal error: unknown field type\n";
            throw pybind11::type_error("internal error: unknown field type");
    }
}

void set_field_to_object(StructStoreField& field, const pybind11::object& value) {
    switch (field.type) {
        case FieldTypeValue::INT:
            (int&) field = pybind11::int_(value);
            break;
        case FieldTypeValue::STRING:
            (structstore::string&) field = std::string(pybind11::str(value));
            break;
        case FieldTypeValue::BOOL:
            (bool&) field = pybind11::bool_(value);
            break;
        case FieldTypeValue::STRUCT:
            // TODO
            // (StructStoreBase&) field = pybind11::cast<StructStoreBase>(value);
            break;
        default:
            std::cerr << "internal error: unknown field type\n";
            throw pybind11::type_error("internal error: unknown field type");
    }
}

pybind11::object to_dict(StructStoreBase& store) {
    auto dict = pybind11::dict();
    for (auto& [key, value]: store.fields) {
        if (value.type == FieldTypeValue::STRUCT) {
            dict[key.str] = to_dict(value);
        } else {
            dict[key.str] = field_to_object(value);
        }
    }
    return dict;
}

}

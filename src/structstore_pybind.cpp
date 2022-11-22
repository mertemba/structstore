#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "structstore.hpp"
#include "structstore_shared.hpp"

namespace structstore {

template<typename T>
pybind11::class_<T> register_pystruct(pybind11::module_& m, const char* name) {
    static_assert(std::is_base_of<StructStore, T>::value, "Template argument must be derived from StructStore");
    pybind11::class_<T> cls = pybind11::class_<T>{m, name};
    cls.def(pybind11::init<>());
    cls.def_readonly("__slots__", &StructStore::slots);
    cls.def("__getattr__", [](T& store, const std::string& name) {
        HashString name_str{name.c_str()};
        auto it = store.fields.find(name_str);
        if (it == store.fields.end()) {
//            std::cerr << "could not find field " << name << "\n";
            throw pybind11::attribute_error("could not find field " + name);
        }
        try {
            return field_to_object(it->second);
        } catch (const std::runtime_error& e) {
            std::cerr << "in getattr on field " << name << ": " << e.what() << "\n";
            throw;
        }
    }, pybind11::return_value_policy::reference_internal);
    cls.def("__setattr__", [](T& store, const std::string& name, pybind11::object value) {
        HashString name_str{name.c_str()};
        auto it = store.fields.find(name_str);
        if (it == store.fields.end()) {
//            std::cerr << "could not find field " << name << "\n";
            throw pybind11::attribute_error("could not find field " + name);
        }
        try {
            return set_field_to_object(it->second, value);
        } catch (const std::runtime_error& e) {
            std::cerr << "in setattr on field " << name << ": " << e.what() << "\n";
            throw;
        }
    });
    cls.def("to_yaml", [](T& store) {
        return YAML::Dump(to_yaml(store));
    });
    cls.def("to_str", [](T& store) {
        return (std::ostringstream() << store).str();
    });
    cls.def("to_dict", [](T& store) {
        return to_dict(store);
    });

    std::string shared_name = name + std::string("Shared");
    auto shcls = pybind11::class_<StructStoreShared>(m, shared_name.c_str());
    shcls.def(pybind11::init<const std::string&>());
    shcls.def(pybind11::init<const std::string&, ssize_t>());
    shcls.def("get_store", &StructStoreShared::operator*, pybind11::return_value_policy::reference_internal);
    return cls;
}

pybind11::object field_to_object(StructStoreField& field) {
    switch (field.get_type()) {
        case FieldTypeValue::INT:
            return pybind11::int_(field.get<int>());
        case FieldTypeValue::DOUBLE:
            return pybind11::float_(field.get<double>());
        case FieldTypeValue::STRING:
            return pybind11::str(field.get<structstore::string>().c_str());
        case FieldTypeValue::BOOL:
            return pybind11::bool_(field.get<bool>());
        case FieldTypeValue::STRUCT:
            return pybind11::cast(field.get<StructStore>(), pybind11::return_value_policy::reference);
        default:
            std::cerr << "internal error: unknown field type\n";
            throw pybind11::type_error("internal error: unknown field type");
    }
}

void set_field_to_object(StructStoreField& field, const pybind11::object& value) {
    switch (field.get_type()) {
        case FieldTypeValue::INT:
            field.get<int>() = pybind11::int_(value);
            break;
        case FieldTypeValue::DOUBLE:
            field.get<double>() = pybind11::float_(value);
            break;
        case FieldTypeValue::STRING:
            field.get<structstore::string>() = std::string(pybind11::str(value));
            break;
        case FieldTypeValue::BOOL:
            field.get<bool>() = pybind11::bool_(value);
            break;
        case FieldTypeValue::STRUCT:
            // TODO
            // (StructStoreBase&) field = pybind11::cast<StructStoreBase>(value);
            break;
        default:
            throw pybind11::type_error("internal error: unknown field type");
    }
}

pybind11::object to_dict(StructStore& store) {
    auto dict = pybind11::dict();
    for (auto& [key, value]: store.fields) {
        if (value.get_type() == FieldTypeValue::STRUCT) {
            dict[key.str] = to_dict(value.get<StructStore>());
        } else {
            dict[key.str] = field_to_object(value);
        }
    }
    return dict;
}

}

PYBIND11_MODULE(structstore, m) {
    auto cls = structstore::register_pystruct<structstore::StructStore>(m, "StructStore");
    cls.def("add_int", &structstore::StructStore::get<int>);
    cls.def("add_float", &structstore::StructStore::get<double>);
    cls.def("add_str", &structstore::StructStore::get<structstore::string>);
    cls.def("add_bool", &structstore::StructStore::get<bool>);
    cls.def("add_store", &structstore::StructStore::get<structstore::StructStore>,
            pybind11::return_value_policy::reference_internal);
}

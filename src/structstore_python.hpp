#ifndef STRUCTSTORE_PYTHON_HPP
#define STRUCTSTORE_PYTHON_HPP

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "structstore.hpp"
#include "structstore_shared.hpp"

pybind11::object field_to_object(StructStoreField& field) {
    switch (field.type) {
        case FieldTypeValue::INT:
            return pybind11::int_((int&) field);
        case FieldTypeValue::STRING:
            return pybind11::str((std::string&) field);
        case FieldTypeValue::ARENA_STR:
            return pybind11::str(((arena_str&) field).c_str());
        case FieldTypeValue::BOOL:
            return pybind11::bool_((bool&) field);
        case FieldTypeValue::STRUCT:
            return pybind11::cast((StructStoreBase&) field, pybind11::return_value_policy::reference);
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
            (std::string&) field = pybind11::str(value);
            break;
        case FieldTypeValue::ARENA_STR:
            (arena_str&) field = std::string(pybind11::str(value));
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

template<typename T>
pybind11::class_<T> register_pystruct(pybind11::module_& m, const char* name, bool register_base) {
    // TODO: register only StructStoreBase?
    static_assert(std::is_base_of<StructStoreBase, T>::value, "Template argument must be derived from StructStoreBase");
    pybind11::object base_cls;
    if (register_base) {
        base_cls = register_pystruct<StructStoreBase>(m, "StructStoreBase", false);
    }
    pybind11::class_<T> cls =
            register_base
            ? pybind11::class_<T>{m, name, base_cls}
            : pybind11::class_<T>{m, name};
    cls.def(pybind11::init<>());
    cls.def_readonly("__slots__", &StructStore<T>::slots);
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
    }, pybind11::return_value_policy::reference);
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
    auto shcls = pybind11::class_<StructStoreShared<T>>(m, shared_name.c_str());
    shcls.def(pybind11::init<const std::string&>());
    shcls.def("get_store", &StructStoreShared<T>::operator*, pybind11::return_value_policy::reference);
    return cls;
}

#endif

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "structstore.hpp"
#include "structstore_shared.hpp"
#include "structstore_containers.hpp"

namespace structstore {

static pybind11::object to_object(FieldAccess access) {
    switch (access.get_type()) {
        case FieldTypeValue::INT:
            return pybind11::int_(access.get<int>());
        case FieldTypeValue::DOUBLE:
            return pybind11::float_(access.get<double>());
        case FieldTypeValue::STRING:
            return pybind11::str(access.get<structstore::string>().c_str());
        case FieldTypeValue::BOOL:
            return pybind11::bool_(access.get<bool>());
        case FieldTypeValue::STRUCT:
            return pybind11::cast(access.get<StructStore>(), pybind11::return_value_policy::reference);
        case FieldTypeValue::EMPTY:
            throw std::runtime_error("trying to read unset field");
        default:
            throw std::runtime_error("this type cannot be stored in a StructStore");
    }
}

static void from_object(FieldAccess access, const pybind11::object& value) {
    if (pybind11::isinstance<pybind11::bool_>(value)) {
        access.get<bool>() = pybind11::bool_(value);
    } else if (pybind11::isinstance<pybind11::int_>(value)) {
        access.get<int>() = pybind11::int_(value);
    } else if (pybind11::isinstance<pybind11::float_>(value)) {
        access.get<double>() = pybind11::float_(value);
    } else if (pybind11::isinstance<pybind11::str>(value)) {
        access.get<structstore::string>() = std::string(pybind11::str(value));
    } else if (pybind11::hasattr(value, "__dict__")) {
        auto dict = pybind11::dict(value.attr("__dict__"));
        for (const auto& [key, val]: dict) {
            std::string key_str = pybind11::str(key);
            from_object(access.get<StructStore>()[key_str.c_str()], dict[key]);
        }
    } else if (pybind11::hasattr(value, "__slots__")) {
        auto slots = pybind11::list(value.attr("__slots__"));
        for (const auto& key: slots) {
            std::string key_str = pybind11::str(key);
            from_object(access.get<StructStore>()[key_str.c_str()], pybind11::getattr(value, key));
        }
    } else {
        std::cerr << "unknown field type " << value.get_type() << std::endl;
        throw pybind11::type_error("internal error: unknown field type");
    }
}

pybind11::object to_dict(StructStore& store) {
    auto dict = pybind11::dict();
    for (auto& [key, value]: store.fields) {
        if (value.get_type() == FieldTypeValue::STRUCT) {
            dict[key.str] = to_dict(value.get<StructStore>());
        } else {
            dict[key.str] = to_object({value, store.mm_alloc});
        }
    }
    return dict;
}

void register_structstore_pybind(pybind11::module_& m) {
    pybind11::class_<StructStore> cls = pybind11::class_<StructStore>{m, "StructStore"};
    cls.def(pybind11::init<>());
    cls.def_readonly("__slots__", &StructStore::slots);
    cls.def("__getattr__", [](StructStore& store, const std::string& name) {
        StructStoreField* field = store.try_get_field(HashString{name.c_str()});
        if (field == nullptr) {
            throw pybind11::attribute_error();
        }
        return to_object({*field, store.mm_alloc});
    }, pybind11::return_value_policy::reference_internal);
    cls.def("__setattr__", [](StructStore& store, const std::string& name, pybind11::object value) {
        from_object(store[name.c_str()], value);
    });
    cls.def("to_yaml", [](StructStore& store) {
        return YAML::Dump(to_yaml(store));
    });
    cls.def("to_str", [](StructStore& store) {
        return (std::ostringstream() << store).str();
    });
    cls.def("to_dict", [](StructStore& store) {
        return to_dict(store);
    });

    auto shcls = pybind11::class_<StructStoreShared>(m, "StructStoreShared");
    shcls.def(pybind11::init<const std::string&>());
    shcls.def(pybind11::init<const std::string&, ssize_t>());
    shcls.def("get_store", &StructStoreShared::operator*, pybind11::return_value_policy::reference_internal);
}

}

PYBIND11_MODULE(structstore, m) {
    structstore::register_structstore_pybind(m);
}

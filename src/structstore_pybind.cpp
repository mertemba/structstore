#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "structstore.hpp"
#include "structstore_shared.hpp"
#include "structstore_containers.hpp"

namespace structstore {

template<bool recursive>
pybind11::object to_list(const List& list);

template<bool recursive>
pybind11::object to_dict(const StructStore& store);

template<bool recursive>
pybind11::object to_object(const StructStoreField& field) {
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
            if constexpr (recursive) {
                return to_dict<true>(field.get<StructStore>());
            } else {
                return pybind11::cast(field.get<StructStore>(), pybind11::return_value_policy::reference);
            }
        case FieldTypeValue::LIST:
            if constexpr (recursive) {
                return to_list<true>(field.get<List>());
            } else {
                return pybind11::cast(field.get<List>(), pybind11::return_value_policy::reference);
            }
        case FieldTypeValue::EMPTY:
            throw std::runtime_error("trying to read unset field");
        default:
            throw std::runtime_error("this type cannot be stored in a StructStore");
    }
}

static void from_object(FieldAccess access, const pybind11::handle& value) {
    if (pybind11::isinstance<pybind11::bool_>(value)) {
        access.get<bool>() = value.cast<bool>();
    } else if (pybind11::isinstance<pybind11::int_>(value)) {
        access.get<int>() = value.cast<int>();
    } else if (pybind11::isinstance<pybind11::float_>(value)) {
        access.get<double>() = value.cast<double>();
    } else if (pybind11::isinstance<pybind11::str>(value)) {
        access.get<structstore::string>() = std::string(pybind11::str(value));
    } else if (pybind11::isinstance<pybind11::list>(value)) {
        List& list = access.get<List>();
        list.clear();
        for (const auto& val: value.cast<pybind11::list>()) {
            from_object(list.push_back(), val);
        }
    } else if (pybind11::hasattr(value, "__dict__")) {
        auto dict = pybind11::dict(value.attr("__dict__"));
        auto& store = access.get<StructStore>();
        for (const auto& [key, val]: dict) {
            std::string key_str = pybind11::str(key);
            from_object(store[key_str.c_str()], dict[key]);
        }
    } else if (pybind11::hasattr(value, "__slots__")) {
        auto slots = pybind11::list(value.attr("__slots__"));
        auto& store = access.get<StructStore>();
        for (const auto& key: slots) {
            std::string key_str = pybind11::str(key);
            from_object(store[key_str.c_str()], pybind11::getattr(value, key));
        }
    } else {
        std::cerr << "unknown field type " << value.get_type() << std::endl;
        throw pybind11::type_error("internal error: unknown field type");
    }
}

template<bool recursive>
pybind11::object to_list(const List& list) {
    auto pylist = pybind11::list();
    for (const StructStoreField& field: list) {
        pylist.append(to_object<recursive>(field));
    }
    return pylist;
}

template<bool recursive>
pybind11::object to_dict(const StructStore& store) {
    auto dict = pybind11::dict();
    for (auto& [key, value]: store.fields) {
        dict[key.str] = to_object<recursive>(value);
    }
    return dict;
}

void register_structstore_pybind(pybind11::module_& m) {
    pybind11::class_<StructStore> cls = pybind11::class_<StructStore>{m, "StructStore"};
    cls.def(pybind11::init<>());
    cls.def_readonly("__slots__", &StructStore::slots);
    cls.def("__getattr__", [](StructStore& store, const std::string& name) {
        auto lock = store.read_lock();
        StructStoreField* field = store.try_get_field(HashString{name.c_str()});
        if (field == nullptr) {
            throw pybind11::attribute_error();
        }
        return to_object<false>(*field);
    }, pybind11::return_value_policy::reference_internal);
    cls.def("__setattr__", [](StructStore& store, const std::string& name, pybind11::object value) {
        auto lock = store.write_lock();
        from_object(store[name.c_str()], value);
    });
    cls.def("to_yaml", [](StructStore& store) {
        auto lock = store.read_lock();
        return YAML::Dump(to_yaml(store));
    });
    cls.def("__repr__", [](StructStore& store) {
        auto lock = store.read_lock();
        return (std::ostringstream() << store).str();
    });
    cls.def("copy", [](StructStore& store) {
        auto lock = store.read_lock();
        return to_dict<false>(store);
    });
    cls.def("deepcopy", [](StructStore& store) {
        auto lock = store.read_lock();
        return to_dict<true>(store);
    });

    auto shcls = pybind11::class_<StructStoreShared>(m, "StructStoreShared");
    shcls.def(pybind11::init<const std::string&>());
    shcls.def(pybind11::init<const std::string&, ssize_t>());
    shcls.def("get_store", &StructStoreShared::operator*, pybind11::return_value_policy::reference_internal);

    auto list = pybind11::class_<List>(m, "StructStoreList");
    list.def("__repr__", [](const List& list) {
        auto lock = list.read_lock();
        return (std::ostringstream() << list).str();
    });
    list.def("copy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<false>(list);
    });
    list.def("deepcopy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<true>(list);
    });
}

}

PYBIND11_MODULE(structstore, m) {
    structstore::register_structstore_pybind(m);
}

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include "structstore.hpp"
#include "structstore_shared.hpp"
#include "structstore_containers.hpp"

namespace structstore {

namespace py = pybind11;

static py::object SimpleNamespace;

template<bool recursive>
py::object to_list(const List& list);

template<bool recursive>
py::object to_object(const StructStore& store);

template<bool recursive>
py::object to_object(const StructStoreField& field) {
    switch (field.get_type()) {
        case FieldTypeValue::INT:
            return py::int_(field.get<int>());
        case FieldTypeValue::DOUBLE:
            return py::float_(field.get<double>());
        case FieldTypeValue::STRING:
            return py::str(field.get<structstore::string>().c_str());
        case FieldTypeValue::BOOL:
            return py::bool_(field.get<bool>());
        case FieldTypeValue::STRUCT:
            if constexpr (recursive) {
                return to_object<true>(field.get<StructStore>());
            } else {
                return py::cast(field.get<StructStore>(), py::return_value_policy::reference);
            }
        case FieldTypeValue::LIST:
            if constexpr (recursive) {
                return to_list<true>(field.get<List>());
            } else {
                return py::cast(field.get<List>(), py::return_value_policy::reference);
            }
        case FieldTypeValue::MATRIX:
            return py::array(py::cast(field.get<Matrix>(), py::return_value_policy::reference));
        case FieldTypeValue::EMPTY:
            return py::none();
        default:
            throw std::runtime_error("this type cannot be stored in a StructStore");
    }
}

static void from_object(FieldAccess access, const py::handle& value) {
    if (py::isinstance<py::bool_>(value)) {
        access.get<bool>() = value.cast<bool>();
    } else if (py::isinstance<py::int_>(value)) {
        access.get<int>() = value.cast<int>();
    } else if (py::isinstance<py::float_>(value)) {
        access.get<double>() = value.cast<double>();
    } else if (py::isinstance<py::str>(value)) {
        access.get<structstore::string>() = std::string(py::str(value));
    } else if (py::isinstance<py::list>(value) || py::isinstance<py::tuple>(value)) {
        List& list = access.get<List>();
        list.clear();
        for (const auto& val: value.cast<py::list>()) {
            from_object(list.push_back(), val);
        }
    } else if (py::isinstance<py::array>(value)) {
        auto array = value.cast<py::array>();
        py::buffer_info info = array.request();
        if (info.format != py::format_descriptor<double>::format()) {
            throw std::runtime_error("Incompatible format: expected a double array!");
        }
        int rows, cols;
        bool is_vector = false;
        if (info.ndim == 1) {
            rows = info.shape[0];
            cols = 1;
            is_vector = true;
        } else if (info.ndim == 2) {
            rows = info.shape[0];
            cols = info.shape[1];
        } else {
            throw std::runtime_error("Incompatible buffer dimension!");
        }
        access.get<Matrix>().from(rows, cols, (double*) info.ptr, is_vector);
    } else if (py::hasattr(value, "__dict__")) {
        auto dict = py::dict(value.attr("__dict__"));
        auto& store = access.get<StructStore>();
        for (const auto& [key, val]: dict) {
            std::string key_str = py::str(key);
            from_object(store[key_str.c_str()], dict[key]);
        }
    } else if (py::hasattr(value, "__slots__")) {
        auto slots = py::list(value.attr("__slots__"));
        auto& store = access.get<StructStore>();
        for (const auto& key: slots) {
            std::string key_str = py::str(key);
            from_object(store[key_str.c_str()], py::getattr(value, key));
        }
    } else if (py::isinstance<py::none>(value)) {
        // do nothing
    } else {
        std::cerr << "unknown field type " << value.get_type() << std::endl;
        throw py::type_error("internal error: unknown field type");
    }
}

template<bool recursive>
py::object to_list(const List& list) {
    auto pylist = py::list();
    for (const StructStoreField& field: list) {
        pylist.append(to_object<recursive>(field));
    }
    return pylist;
}

template<bool recursive>
py::object to_object(const StructStore& store) {
    auto obj = SimpleNamespace();
    for (const auto& name: store.slots) {
        py::setattr(obj, name.str, to_object<recursive>(store.fields.at(name)));
    }
    return obj;
}

void register_structstore_pybind(py::module_& m) {
    SimpleNamespace = py::module_::import("types").attr("SimpleNamespace");

    py::class_<StructStore> cls = py::class_<StructStore>{m, "StructStore"};
    cls.def(py::init<>());
    cls.def_property_readonly("__slots__", [](StructStore& store) {
        auto ret = py::list();
        for (const auto& str: store.slots) {
            ret.append(str.str);
        }
        return ret;
    });
    cls.def("__getattr__", [](StructStore& store, const std::string& name) {
        auto lock = store.read_lock();
        StructStoreField* field = store.try_get_field(HashString{name.c_str()});
        if (field == nullptr) {
            throw py::attribute_error();
        }
        return to_object<false>(*field);
    }, py::return_value_policy::reference_internal);
    cls.def("__setattr__", [](StructStore& store, const std::string& name, py::object value) {
        auto lock = store.write_lock();
        from_object(store[name.c_str()], value);
    });
    cls.def("to_yaml", [](StructStore& store) {
        auto lock = store.read_lock();
        return YAML::Dump(to_yaml(store));
    });
    cls.def("__repr__", [](StructStore& store) {
        auto lock = store.read_lock();
        std::ostringstream str;
        str << store;
        return str.str();
    });
    cls.def("copy", [](StructStore& store) {
        auto lock = store.read_lock();
        return to_object<false>(store);
    });
    cls.def("deepcopy", [](StructStore& store) {
        auto lock = store.read_lock();
        return to_object<true>(store);
    });
    cls.def("size", [](StructStore& store) {
        return store.mm_alloc.get_size();
    });
    cls.def("allocated", [](StructStore& store) {
        return store.mm_alloc.get_allocated();
    });

    auto shcls = py::class_<StructStoreShared>(m, "StructStoreShared");
    shcls.def(py::init<const std::string&, ssize_t, bool>(),
              py::arg("path"),
              py::arg("size") = 2048,
              py::arg("owning") = false);
    shcls.def("valid", &StructStoreShared::valid);
    shcls.def("revalidate", [](StructStoreShared& shs, bool block) {
                bool res = false;
                do {
                    // necessary to get out of the loop on interrupting signal
                    if (PyErr_CheckSignals() != 0) {
                        throw py::error_already_set();
                    }
                    res = shs.revalidate(false);
                } while (res == false && block);
                return res;
              },
              py::arg("block") = true);
    shcls.def("get_store", &StructStoreShared::operator*, py::return_value_policy::reference_internal);

    auto list = py::class_<List>(m, "StructStoreList");
    list.def("__repr__", [](const List& list) {
        auto lock = list.read_lock();
        std::ostringstream str;
        str << list;
        return str.str();
    });
    list.def("copy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<false>(list);
    });
    list.def("deepcopy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<true>(list);
    });

    auto matrix = py::class_<Matrix>(m, "StructStoreMatrix", py::buffer_protocol());
    matrix.def_buffer([](Matrix& m) -> py::buffer_info {
        if (m.is_vector()) {
            return py::buffer_info(
                    m.data(), sizeof(double),
                    py::format_descriptor<double>::format(),
                    1, {m.rows() * m.cols()},
                    {sizeof(double)}
            );
        }
        return py::buffer_info(
                m.data(), sizeof(double),
                py::format_descriptor<double>::format(),
                2, {m.rows(), m.cols()},
                {sizeof(double) * m.cols(), sizeof(double)}
        );
    });
}

}

PYBIND11_MODULE(structstore, m) {
    structstore::register_structstore_pybind(m);
}

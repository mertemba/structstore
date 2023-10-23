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
        case FieldTypeValue::MATRIX: {
            Matrix& m = field.get<Matrix>();
            // in the recursive case use empty capsule to avoid copy
            // then memory lifetime is managed by structstore field
            auto parent = recursive ? py::handle() : py::capsule([](){}).release();

            py::array::ShapeContainer shape;
            if (m.is_vector()) {
                shape = {m.rows() * m.cols()};
            } else {
                shape = {m.rows(), m.cols()};
            }

            return py::array_t<double>(shape, m.data(), parent);
        }
        case FieldTypeValue::EMPTY:
            return py::none();
        default:
            throw std::runtime_error("this type cannot be stored in a StructStore");
    }
}

static void from_object(FieldAccess access, const py::handle& value, const std::string& field_name) {
    if (py::isinstance<List>(value)
            && access.get_type() == FieldTypeValue::LIST) {
        if (&value.cast<List&>() == &access.get<List>()) {
            return;
        }
    }
    if (py::isinstance<StructStore>(value)
            && access.get_type() == FieldTypeValue::STRUCT) {
        if (&value.cast<StructStore&>() == &access.get<StructStore>()) {
            return;
        }
    }
    if (py::isinstance<Matrix>(value)
            && access.get_type() == FieldTypeValue::MATRIX) {
        if (&value.cast<Matrix&>() == &access.get<Matrix>()) {
            return;
        }
    }
    if (py::isinstance<py::bool_>(value)) {
        access.set_type<bool>();
        access.get<bool>() = value.cast<bool>();
    } else if (py::isinstance<py::int_>(value)) {
        access.set_type<int>();
        access.get<int>() = value.cast<int>();
    } else if (py::isinstance<py::float_>(value)) {
        access.set_type<double>();
        access.get<double>() = value.cast<double>();
    } else if (py::isinstance<py::str>(value)) {
        access.set_type<structstore::string>();
        access.get<structstore::string>() = std::string(py::str(value));
    } else if (py::isinstance<py::list>(value)
            || py::isinstance<py::tuple>(value)
            || py::isinstance<List>(value)) {
        access.set_type<List>();
        List& list = access.get<List>();
        list.clear();
        int i = 0;
        for (const auto& val: value.cast<py::list>()) {
            from_object(list.push_back(), val, std::to_string(i));
            ++i;
        }
    } else if (py::isinstance<py::array>(value)) {
        access.set_type<Matrix>();
        auto array = py::array_t<double, py::array::c_style | py::array::forcecast>::ensure(value);
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
        access.set_type<StructStore>();
        auto dict = py::dict(value.attr("__dict__"));
        StructStore& store = access.get<StructStore>();
        store.clear();
        for (const auto& [key, val]: dict) {
            std::string key_str = py::str(key);
            from_object(store[key_str.c_str()], dict[key], py::str(key));
        }
    } else if (py::hasattr(value, "__slots__")) {
        access.set_type<StructStore>();
        auto slots = py::list(value.attr("__slots__"));
        StructStore& store = access.get<StructStore>();
        store.clear();
        for (const auto& key: slots) {
            std::string key_str = py::str(key);
            from_object(store[key_str.c_str()], py::getattr(value, key), py::str(key));
        }
    } else if (py::isinstance<py::dict>(value)) {
        access.set_type<StructStore>();
        py::dict dict = py::cast<py::dict>(value);
        StructStore& store = access.get<StructStore>();
        store.clear();
        for (auto& [key, val]: dict) {
            if (!py::isinstance<py::str>(key)) {
                std::ostringstream msg;
                msg << "Key '" << py::str(key)
                    << "' has unsupported type '" << py::str(key.get_type())
                    << "'! Only string keys are supported.";
                throw py::type_error(msg.str());
            }
            std::string key_str = py::str(key);
            from_object(store[key_str.c_str()], dict[key], key_str);
        }
    } else if (py::isinstance<py::none>(value)) {
        access.clear();
    } else {
        std::ostringstream msg;
        msg << "field '" << field_name << "' has unsupported type '" << py::str(value.get_type()) << "'";
        throw py::type_error(msg.str());
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

class SpinLockContextManager {

    SpinMutex* mutex;
    bool locked;

public:

    explicit SpinLockContextManager (SpinMutex* mutex)
        : mutex{mutex}, locked{true} { }

    SpinLockContextManager (SpinLockContextManager&& other)
        : mutex{other.mutex}, locked{other.locked} {
        other.locked = false;
    }

    SpinLockContextManager(const SpinLockContextManager&) = delete;
    SpinLockContextManager& operator=(SpinLockContextManager&& other) = delete;
    SpinLockContextManager& operator=(const SpinLockContextManager&) = delete;

    void exit (py::handle&, py::handle&, py::handle&) {
        if (locked) {
            locked = false;
            mutex->unlock();
        }
    }

    ~SpinLockContextManager () {
        if (locked) {
            mutex->unlock();
        }
    }
};

template<typename T>
void register_structstore_methods(py::class_<T>& cls) {
    cls.def_property_readonly("__slots__", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto ret = py::list();
        for (const auto& str: store.get_slots()) {
            ret.append(str.str);
        }
        return ret;
    });
    auto get_field = [](T& t, const std::string& name) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        StructStoreField* field = store.try_get_field(HashString{name.c_str()});
        if (field == nullptr) {
            throw py::attribute_error();
        }
        return to_object<false>(*field);
    };
    auto set_field = [](T& t, const std::string& name, py::object value) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.write_lock();
        from_object(store[name.c_str()], value, name);
    };
    cls.def("__getattr__", get_field, py::return_value_policy::reference_internal);
    cls.def("__setattr__", set_field);
    cls.def("__getitem__", get_field, py::return_value_policy::reference_internal);
    cls.def("__setitem__", set_field);
    cls.def("lock", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        store.get_mutex().lock();
        return SpinLockContextManager(&store.get_mutex());
    }, py::return_value_policy::move);
    cls.def("to_yaml", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        return YAML::Dump(to_yaml(store));
    });
    cls.def("__repr__", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        std::ostringstream str;
        str << store;
        return str.str();
    });
    cls.def("copy", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        return to_object<false>(store);
    });
    cls.def("deepcopy", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        return to_object<true>(store);
    });
    cls.def("__copy__", [](T& t, py::handle&) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        return to_object<false>(store);
    });
    cls.def("__deepcopy__", [](T& t, py::handle&) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        return to_object<true>(store);
    });
    cls.def("size", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        return store.mm_alloc.get_size();
    });
    cls.def("allocated", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        return store.mm_alloc.get_allocated();
    });
    cls.def("clear", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.write_lock();
        store.clear();
    });
}

void register_structstore_pybind(py::module_& m) {

    SimpleNamespace = py::module_::import("types").attr("SimpleNamespace");

    py::class_<SpinLockContextManager>(m, "SpinLockContextManager")
        .def("__enter__", [](SpinLockContextManager& cm){})
        .def("__exit__", &SpinLockContextManager::exit);

    py::class_<StructStore> cls = py::class_<StructStore>{m, "StructStore"};
    cls.def(py::init<>());
    register_structstore_methods(cls);

    py::enum_<CleanupMode>(m, "CleanupMode")
            .value("NEVER", NEVER)
            .value("IF_LAST", IF_LAST)
            .value("ALWAYS", ALWAYS)
            .export_values();

    auto shcls = py::class_<StructStoreShared>(m, "StructStoreShared");
    register_structstore_methods(shcls);
    shcls.def(py::init([](const std::string& path, size_t size, bool reinit, bool use_file, CleanupMode cleanup,
                          uintptr_t target_addr) {
                  return StructStoreShared{path, size, reinit, use_file, cleanup, (void*) target_addr};
              }),
              py::arg("path"),
              py::arg("size") = 2048,
              py::arg("reinit") = false,
              py::arg("use_file") = false,
              py::arg("cleanup") = IF_LAST,
              py::arg("target_addr") = 0);
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
    shcls.def("addr", [](StructStoreShared& shs) {
        return uintptr_t(shs.addr());
    });
    shcls.def("to_bytes", [](StructStoreShared& shs) {
        return py::bytes((const char*) shs.addr(), shs.size());
    });
    shcls.def("from_bytes", [](StructStoreShared& shs, py::bytes buffer) {
        std::string str = buffer;
        shs.from_buffer(str.data(), str.size());
    });

    auto list = py::class_<List>(m, "StructStoreList");
    list.def("__repr__", [](const List& list) {
        auto lock = list.read_lock();
        std::ostringstream str;
        str << list;
        return str.str();
    });
    list.def("lock", [](List& list) {
        list.get_mutex().lock();
        return SpinLockContextManager(&list.get_mutex());
    }, py::return_value_policy::move);
    list.def("__len__", [](List& list) {
        auto lock = list.read_lock();
        return list.size();
    });
    list.def("insert", [](List& list, size_t index, py::handle& value) {
        auto lock = list.write_lock();
        FieldAccess access = list.insert(index);
        from_object(access, value, std::to_string(index));
    });
    list.def("extend", [](List& list, py::handle& value) {
        auto lock = list.write_lock();
        for (const auto& val : value.cast<py::list>()) {
            std::string field_name = std::to_string(list.size());
            from_object(list.push_back(), val, field_name);
        }
    });
    list.def("append", [](List& list, py::handle& value) {
        auto lock = list.write_lock();
        from_object(list.push_back(), value, std::to_string(list.size() - 1));
    });
    list.def("pop", [](List& list, size_t index) {
        auto lock = list.write_lock();
        const auto& res = to_object<true>(list[index].get_field());
        list.erase(index);
        return res;
    });
    list.def("__add__", [](List& list, py::handle& value) {
        auto lock = list.read_lock();
        return to_list<false>(list) + value.cast<py::list>();
    });
    list.def("__iadd__", [](List& list, py::handle& value) {
        auto lock = list.write_lock();
        for (const auto& val : value.cast<py::list>()) {
            from_object(list.push_back(), val, std::to_string(list.size() - 1));
        }
        return to_list<false>(list);
    });
    list.def("__setitem__", [](List& list, size_t index, py::handle& value) {
        auto lock = list.write_lock();
        from_object(list[index], value, std::to_string(index));
    });
    list.def("__getitem__", [](List& list, size_t index) {
        auto lock = list.read_lock();
        return to_object<false>(list[index].get_field());
    });
    list.def("__delitem__", [](List& list, size_t index) {
        auto lock = list.write_lock();
        list.erase(index);
    });
    list.def("__iter__", [](List& list) {
        auto lock = list.read_lock();
        return pybind11::make_iterator(list.begin(), list.end());
    }, py::keep_alive<0, 1>());
    list.def("copy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<false>(list);
    });
    list.def("deepcopy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<true>(list);
    });
    list.def("__copy__", [](const List& list, py::handle&) {
        auto lock = list.read_lock();
        return to_list<false>(list);
    });
    list.def("__deepcopy__", [](const List& list, py::handle&) {
        auto lock = list.read_lock();
        return to_list<true>(list);
    });
    list.def("clear", [](List& list) {
        auto lock = list.write_lock();
        list.clear();
    });
}

}

// required to make __iter__ method work
namespace pybind11 {
    namespace detail {
        template <> struct type_caster<structstore::StructStoreField> {
        public:
            PYBIND11_TYPE_CASTER(structstore::StructStoreField, const_name("StructStoreField"));
            static handle cast(structstore::StructStoreField& src, return_value_policy, handle) {
                return structstore::to_object<false>(src).release();
            }
        };
    }
}

PYBIND11_MODULE(structstore, m) {
    structstore::register_structstore_pybind(m);
}

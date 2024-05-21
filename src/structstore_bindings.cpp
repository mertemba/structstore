#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>
#include "structstore/structstore.hpp"

namespace structstore {

namespace nb = nanobind;

static nb::object SimpleNamespace;

template<bool recursive>
nb::object to_list(const List& list);

template<bool recursive>
nb::object to_object(const StructStoreField& field) {
    switch (field.get_type()) {
        case FieldTypeValue::INT:
            return nb::int_(field.get<int>());
        case FieldTypeValue::DOUBLE:
            return nb::float_(field.get<double>());
        case FieldTypeValue::STRING:
            return nb::str(field.get<structstore::string>().c_str());
        case FieldTypeValue::BOOL:
            return nb::bool_(field.get<bool>());
        case FieldTypeValue::STRUCT:
            if constexpr (recursive) {
                return to_object<true>(field.get<StructStore>());
            } else {
                return nb::cast(field.get<StructStore>(), nb::rv_policy::reference);
            }
        case FieldTypeValue::LIST:
            if constexpr (recursive) {
                return to_list<true>(field.get<List>());
            } else {
                return nb::cast(field.get<List>(), nb::rv_policy::reference);
            }
        case FieldTypeValue::MATRIX: {
            Matrix& m = field.get<Matrix>();
            // todo: in the recursive case use empty capsule to avoid copy?
            // then memory lifetime is managed by structstore field
            // auto parent = recursive ? nb::handle() : nb::capsule([]() {}).release();
            auto parent = nb::handle();
            return nb::cast(nb::ndarray<double, nb::c_contig, nb::numpy>(m.data(), m.ndim(), m.shape(), parent));
        }
        case FieldTypeValue::EMPTY:
            return nb::none();
        default:
            throw std::runtime_error("this type cannot be stored in a StructStore");
    }
}

static void from_object(FieldAccess access, const nb::handle& value, const std::string& field_name) {
    if (nb::isinstance<List>(value)
            && access.get_type() == FieldTypeValue::LIST) {
        if (&nb::cast<List&>(value) == &access.get<List>()) {
            return;
        }
    }
    if (nb::isinstance<StructStore>(value)
            && access.get_type() == FieldTypeValue::STRUCT) {
        if (&nb::cast<StructStore&>(value) == &access.get<StructStore>()) {
            return;
        }
    }
    if (nb::isinstance<Matrix>(value)
            && access.get_type() == FieldTypeValue::MATRIX) {
        if (&nb::cast<Matrix&>(value) == &access.get<Matrix>()) {
            return;
        }
    }
    if (nb::isinstance<nb::bool_>(value)) {
        access.set_type<bool>();
        access.get<bool>() = nb::cast<bool>(value);
    } else if (nb::isinstance<nb::int_>(value)) {
        access.set_type<int>();
        access.get<int>() = nb::cast<int>(value);
    } else if (nb::isinstance<nb::float_>(value)) {
        access.set_type<double>();
        access.get<double>() = nb::cast<double>(value);
    } else if (nb::isinstance<nb::str>(value)) {
        access.set_type<structstore::string>();
        access.get<structstore::string>() = nb::cast<std::string>(value);
    } else if (nb::ndarray_check(value)) {
        access.set_type<Matrix>();
        auto array = nb::cast<nb::ndarray<const double, nb::c_contig>>(value);
        if (array.ndim() > Matrix::MAX_DIMS) {
            throw std::runtime_error("Incompatible buffer dimension!");
        }
        access.get<Matrix>().from(array.ndim(), (const size_t*) array.shape_ptr(), array.data());
    } else if (nb::hasattr(value, "__dict__")) {
        access.set_type<StructStore>();
        auto dict = nb::dict(value.attr("__dict__"));
        StructStore& store = access.get<StructStore>();
        store.clear();
        for (const auto& [key, val]: dict) {
            std::string key_str = nb::cast<std::string>(key);
            from_object(store[key_str.c_str()], dict[key], key_str);
        }
    } else if (nb::hasattr(value, "__slots__")) {
        access.set_type<StructStore>();
        auto slots = nb::iterable(value.attr("__slots__"));
        StructStore& store = access.get<StructStore>();
        store.clear();
        for (const auto& key: slots) {
            std::string key_str = nb::cast<std::string>(key);
            from_object(store[key_str.c_str()], nb::getattr(value, key), key_str);
        }
    } else if (nb::isinstance<nb::dict>(value)) {
        access.set_type<StructStore>();
        nb::dict dict = nb::cast<nb::dict>(value);
        StructStore& store = access.get<StructStore>();
        store.clear();
        for (const auto& [key, val]: dict) {
            if (!nb::isinstance<nb::str>(key)) {
                std::ostringstream msg;
                msg << "Key '" << nb::cast<std::string>(nb::str(key))
                    << "' has unsupported type '" << nb::cast<std::string>(nb::str(key.type()))
                    << "'! Only string keys are supported.";
                throw nb::type_error(msg.str().c_str());
            }
            std::string key_str = nb::cast<std::string>(key);
            from_object(store[key_str.c_str()], dict[key], key_str);
        }
    } else if (value.is_none()) {
        access.clear();
    } else if (nb::isinstance<nb::iterable>(value)) {
        access.set_type<List>();
        List& list = access.get<List>();
        list.clear();
        int i = 0;
        for (const auto& val: nb::cast<nb::iterable>(value)) {
            from_object(list.push_back(), val, std::to_string(i));
            ++i;
        }
    } else {
        std::ostringstream msg;
        msg << "field '" << field_name << "' has unsupported type '" << nb::cast<std::string>(nb::str(value.type()))
            << "'";
        throw nb::type_error(msg.str().c_str());
    }
}

template<bool recursive>
nb::object to_list(const List& list) {
    auto pylist = nb::list();
    for (const StructStoreField& field: list) {
        pylist.append(to_object<recursive>(field));
    }
    return pylist;
}

template<bool recursive>
nb::object to_object(const StructStore& store) {
    auto obj = SimpleNamespace();
    for (const auto& name: store.slots) {
        nb::setattr(obj, name.str, to_object<recursive>(store.fields.at(name)));
    }
    return obj;
}

class SpinLockContextManager {

    SpinMutex* mutex = nullptr;

public:

    explicit SpinLockContextManager(SpinMutex* mutex) : mutex{mutex} {}

    SpinLockContextManager(SpinLockContextManager&& other) noexcept {
        std::swap(mutex, other.mutex);
    }

    SpinLockContextManager(const SpinLockContextManager&) = delete;
    SpinLockContextManager& operator=(SpinLockContextManager&& other) = delete;
    SpinLockContextManager& operator=(const SpinLockContextManager&) = delete;

    void unlock() {
        if (mutex) {
            mutex->unlock();
            mutex = nullptr;
        }
    }

    ~SpinLockContextManager() {
        unlock();
    }
};

template<typename T>
void register_structstore_methods(nb::class_<T>& cls) {
    cls.def_prop_ro("__slots__", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto ret = nb::list();
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
            throw nb::attribute_error();
        }
        return to_object<false>(*field);
    };
    auto set_field = [](T& t, const std::string& name, const nb::object& value) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.write_lock();
        from_object(store[name.c_str()], value, name);
    };
    cls.def("__getattr__", get_field, nb::arg("name"), nb::rv_policy::reference_internal);
    cls.def("__setattr__", set_field, nb::arg("name"), nb::arg("value").none());
    cls.def("__getitem__", get_field, nb::arg("name"), nb::rv_policy::reference_internal);
    cls.def("__setitem__", set_field, nb::arg("name"), nb::arg("value").none());
    cls.def("lock", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        store.get_mutex().lock();
        return SpinLockContextManager(&store.get_mutex());
    }, nb::rv_policy::move);
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
    cls.def("__copy__", [](T& t) {
        StructStore& store = static_cast<StructStore&>(t);
        auto lock = store.read_lock();
        return to_object<false>(store);
    });
    cls.def("__deepcopy__", [](T& t, nb::handle&) {
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

NB_MODULE(MODULE_NAME, m) {
    m.attr("__version__") = VERSION_INFO;

    SimpleNamespace = nb::module_::import_("types").attr("SimpleNamespace");

    nb::class_<SpinLockContextManager>(m, "SpinLockContextManager")
            .def("__enter__", [](SpinLockContextManager&) {})
            .def("__exit__", [](SpinLockContextManager& con_man, nb::handle, nb::handle, nb::handle) {
                con_man.unlock();
            }, nb::arg().none(), nb::arg().none(), nb::arg().none());

    nb::class_<StructStore> cls = nb::class_<StructStore>{m, "StructStore"};
    cls.def(nb::init<>());
    register_structstore_methods(cls);

    nb::enum_<CleanupMode>(m, "CleanupMode")
            .value("NEVER", NEVER)
            .value("IF_LAST", IF_LAST)
            .value("ALWAYS", ALWAYS)
            .export_values();

    auto shcls = nb::class_<StructStoreShared>(m, "StructStoreShared");
    register_structstore_methods(shcls);
    shcls.def("__init__",
              [](StructStoreShared* s, const std::string& path, size_t size, bool reinit, bool use_file,
                 CleanupMode cleanup, uintptr_t target_addr) {
                  new(s) StructStoreShared{path, size, reinit, use_file, cleanup, (void*) target_addr};
              },
              nb::arg("path"),
              nb::arg("size") = 2048,
              nb::arg("reinit") = false,
              nb::arg("use_file") = false,
              nb::arg("cleanup") = IF_LAST,
              nb::arg("target_addr") = 0);
    shcls.def("__init__", [](StructStoreShared* s, int fd, bool init) {
                  new(s) StructStoreShared{fd, init};
              },
              nb::arg("fd"),
              nb::arg("init"));
    shcls.def("valid", &StructStoreShared::valid);
    shcls.def("revalidate", [](StructStoreShared& shs, bool block) {
                  bool res = false;
                  do {
                      // necessary to get out of the loop on interrupting signal
                      if (PyErr_CheckSignals() != 0) {
                          throw nb::python_error();
                      }
                      res = shs.revalidate(false);
                  } while (res == false && block);
                  return res;
              },
              nb::arg("block") = true);
    shcls.def("addr", [](StructStoreShared& shs) {
        return uintptr_t(shs.addr());
    });
    shcls.def("to_bytes", [](StructStoreShared& shs) {
        return nb::bytes((const char*) shs.addr(), shs.size());
    });
    shcls.def("from_bytes", [](StructStoreShared& shs, const nb::bytes& buffer) {
        shs.from_buffer((void*) buffer.c_str(), buffer.size());
    });
    shcls.def("close", &StructStoreShared::close);

    auto list = nb::class_<List>(m, "StructStoreList");
    list.def("__repr__", [](const List& list) {
        auto lock = list.read_lock();
        std::ostringstream str;
        str << list;
        return str.str();
    });
    list.def("lock", [](List& list) {
        list.get_mutex().lock();
        return SpinLockContextManager(&list.get_mutex());
    }, nb::rv_policy::move);
    list.def("__len__", [](List& list) {
        auto lock = list.read_lock();
        return list.size();
    });
    list.def("insert", [](List& list, size_t index, nb::handle& value) {
        auto lock = list.write_lock();
        FieldAccess access = list.insert(index);
        from_object(access, value, std::to_string(index));
    }, nb::arg("index"), nb::arg("value").none());
    list.def("extend", [](List& list, nb::iterable& value) {
        auto lock = list.write_lock();
        for (const auto& val: value) {
            std::string field_name = std::to_string(list.size());
            from_object(list.push_back(), val, field_name);
        }
    });
    list.def("append", [](List& list, nb::handle& value) {
        auto lock = list.write_lock();
        from_object(list.push_back(), value, std::to_string(list.size() - 1));
    }, nb::arg("value").none());
    list.def("pop", [](List& list, size_t index) {
        auto lock = list.write_lock();
        const auto& res = to_object<true>(list[index].get_field());
        list.erase(index);
        return res;
    });
    list.def("__add__", [](List& list, nb::list& value) {
        auto lock = list.read_lock();
        return to_list<false>(list) + value;
    });
    list.def("__iadd__", [](List& list, nb::list& value) {
        auto lock = list.write_lock();
        for (const auto& val: value) {
            from_object(list.push_back(), val, std::to_string(list.size() - 1));
        }
        return to_list<false>(list);
    });
    list.def("__setitem__", [](List& list, size_t index, nb::handle& value) {
        auto lock = list.write_lock();
        from_object(list[index], value, std::to_string(index));
    }, nb::arg("index"), nb::arg("value").none());
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
        return nb::make_iterator(nb::type<List>(), "list_iter", list.begin(), list.end());
    }, nb::keep_alive<0, 1>());
    list.def("copy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<false>(list);
    });
    list.def("deepcopy", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<true>(list);
    });
    list.def("__copy__", [](const List& list) {
        auto lock = list.read_lock();
        return to_list<false>(list);
    });
    list.def("__deepcopy__", [](const List& list, nb::handle&) {
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
namespace nanobind::detail {
template<>
struct type_caster<structstore::StructStoreField> {
public:
    NB_TYPE_CASTER(structstore::StructStoreField, const_name("StructStoreField"))

    static handle from_cpp(structstore::StructStoreField& src, rv_policy, cleanup_list*) {
        return structstore::to_object<false>(src).release();
    }
};
}

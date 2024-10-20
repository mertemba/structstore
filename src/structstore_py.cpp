#include "structstore/structstore.hpp"
#include "structstore/stst_py.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>

using namespace structstore;

namespace nb = nanobind;

NB_MODULE(MODULE_NAME, m) {
    m.attr("__version__") = VERSION_INFO;

    // API types:

    nb::class_<ScopedLock>(m, "ScopedLock")
            .def("__enter__", [](ScopedLock&) {})
            .def("__exit__", [](ScopedLock& con_man, nb::handle, nb::handle, nb::handle) {
                con_man.unlock();
            }, nb::arg().none(), nb::arg().none(), nb::arg().none());

    nb::enum_<CleanupMode>(m, "CleanupMode")
            .value("NEVER", NEVER)
            .value("IF_LAST", IF_LAST)
            .value("ALWAYS", ALWAYS)
            .export_values();

    // structstore::StructStore
    nb::class_<StructStore> cls = nb::class_<StructStore>{m, "StructStore"};
    cls.def("__init__", [](StructStore* store) {
        new (store) StructStore(static_alloc);
    });
    py::ToPythonFn structstore_to_python_fn = [](const StructStoreField& field, py::ToPythonMode mode) -> nb::object {
        auto& store = field.get<StructStore>();
        auto dict = nb::dict();
        for (HashString str: store.get_slots()) {
            auto key = nb::cast<std::string>(str.str);
            if (mode == py::ToPythonMode::RECURSIVE) {
                dict[key] = py::to_python(store.at(str), py::ToPythonMode::RECURSIVE);
            } else { // non-recursive convert
                dict[key] = py::to_python_cast(store.at(str));
            }
        }
        return dict;
    };
    py::FromPythonFn structstore_from_python_fn = [](FieldAccess access, const nanobind::handle& value) {
        if (py::copy_cast_from_python<StructStore>(access, value)) {
            return true;
        }
        // todo: strict check if this is one of dict, SimpleNamespace, StructStore
        if (nb::hasattr(value, "__dict__")) {
            auto dict = nb::dict(value.attr("__dict__"));
            StructStore& store = access.get<StructStore>();
            STST_LOG_DEBUG() << "copying __dict__ to " << &store;
            store.clear();
            for (const auto& [key, val]: dict) {
                std::string key_str = nb::cast<std::string>(key);
                py::from_python(store[key_str.c_str()], dict[key], key_str);
            }
            return true;
        }
        if (nb::hasattr(value, "__slots__")) {
            auto slots = nb::iterable(value.attr("__slots__"));
            StructStore& store = access.get<StructStore>();
            STST_LOG_DEBUG() << "copying __slots__ to " << &store;
            store.clear();
            for (const auto& key: slots) {
                std::string key_str = nb::cast<std::string>(key);
                py::from_python(store[key_str.c_str()], nb::getattr(value, key), key_str);
            }
            return true;
        }
        if (nb::isinstance<nb::dict>(value)) {
            nb::dict dict = nb::cast<nb::dict>(value);
            StructStore& store = access.get<StructStore>();
            STST_LOG_DEBUG() << "copying nb::dict to " << &store;
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
                py::from_python(store[key_str.c_str()], dict[key], key_str);
            }
            return true;
        }
        return false;
    };
    py::register_type<StructStore>(structstore_from_python_fn, structstore_to_python_fn,
                                   py::default_to_python_cast_fn<StructStore>);
    py::register_structstore_funcs(cls);

    auto shcls = nb::class_<StructStoreShared>(m, "StructStoreShared");
    py::register_structstore_funcs(shcls);
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
    shcls.def_prop_ro("store", [](StructStoreShared& store) -> StructStore& {
        return *store;
    });

    // built-in field types:

    // basic types
    typing::register_common_types();
    py::register_basic_type<int, nb::int_>();
    py::register_basic_type<double, nb::float_>();
    py::register_basic_type<bool, nb::bool_>();

    // structstore::string
    py::register_type<structstore::string>(
            [](FieldAccess access, const nb::handle& value) {
                if (nb::isinstance<nb::str>(value)) {
                    access.get<structstore::string>() = nb::cast<std::string>(value);
                    return true;
                }
                return false;
            },
            [](const StructStoreField& field, py::ToPythonMode) -> nb::object {
                return nb::str(field.get<structstore::string>().c_str());
            },
            [](const StructStoreField& field) -> nb::object {
                return nb::str(field.get<structstore::string>().c_str());
            });

    // structstore::List
    auto list_cls = nb::class_<List>(m, "StructStoreList");
    py::ToPythonFn list_to_python_fn = [](const StructStoreField& field, py::ToPythonMode mode) -> nb::object {
        auto& list = field.get<List>();
        auto ret = nb::list();
        for (StructStoreField& field_: list) {
            if (mode == py::ToPythonMode::RECURSIVE) {
                ret.append(py::to_python(field_, py::ToPythonMode::RECURSIVE));
            } else { // non-recursive convert
                ret.append(py::to_python_cast(field_));
            }
        }
        return ret;
    };
    py::FromPythonFn list_from_python_fn = [](FieldAccess access, const nanobind::handle& value) {
        if (py::copy_cast_from_python<List>(access, value)) {
            return true;
        }
        if (nb::isinstance<nb::list>(value) || nb::isinstance<nb::tuple>(value)) {
            List& list = access.get<List>();
            list.clear();
            int i = 0;
            for (const auto& val: nb::cast<nb::iterable>(value)) {
                py::from_python(list.push_back(), val, std::to_string(i));
                ++i;
            }
            return true;
        }
        return false;
    };
    py::register_type<List>(list_from_python_fn, list_to_python_fn,
                            py::default_to_python_cast_fn<List>);
    py::register_complex_type_funcs<List>(list_cls);
    list_cls.def("lock", [](List& list) {
        return ScopedLock(list.get_mutex());
    }, nb::rv_policy::move);
    list_cls.def("__len__", [](List& list) {
        auto lock = list.read_lock();
        return list.size();
    });
    list_cls.def("insert", [](List& list, size_t index, nb::handle& value) {
        auto lock = list.write_lock();
        FieldAccess access = list.insert(index);
        py::from_python(access, value, std::to_string(index));
    }, nb::arg("index"), nb::arg("value").none());
    list_cls.def("extend", [](List& list, nb::iterable& value) {
        auto lock = list.write_lock();
        for (const auto& val: value) {
            std::string field_name = std::to_string(list.size());
            py::from_python(list.push_back(), val, field_name);
        }
    });
    list_cls.def("append", [](List& list, nb::handle& value) {
        auto lock = list.write_lock();
        FieldAccess access = list.push_back();
        py::from_python(access, value, std::to_string(list.size() - 1));
    }, nb::arg("value").none());
    list_cls.def("pop", [](List& list, size_t index) {
        auto lock = list.write_lock();
        // todo: this returns a recursive copy, is this wanted?
        auto res = py::to_python(list[index].get_field(), py::ToPythonMode::RECURSIVE);
        list.erase(index);
        return res;
    });
    list_cls.def("__add__", [=](List& list, nb::list& value) {
        auto lock = list.read_lock();
        // todo: this returns a recursive copy, is this wanted?
        return py::to_python(*FieldView{list}, py::ToPythonMode::RECURSIVE) + value;
    });
    list_cls.def("__iadd__", [=](List& list, nb::list& value) {
        auto lock = list.write_lock();
        for (const auto& val: value) {
            py::from_python(list.push_back(), val, std::to_string(list.size() - 1));
        }
        return py::to_python_cast(*FieldView{list});
    });
    list_cls.def("__setitem__", [](List& list, size_t index, nb::handle& value) {
        auto lock = list.write_lock();
        py::from_python(list[index], value, std::to_string(index));
    }, nb::arg("index"), nb::arg("value").none());
    list_cls.def("__getitem__", [](List& list, size_t index) {
        auto lock = list.read_lock();
        STST_LOG_DEBUG() << "getting list item of type " << typing::get_type_name(list[index].get_field().get_type_hash());
        return py::to_python_cast(list[index].get_field());
    });
    list_cls.def("__delitem__", [](List& list, size_t index) {
        auto lock = list.write_lock();
        list.erase(index);
    });
    list_cls.def("__iter__", [](List& list) {
        auto lock = list.read_lock();
        return nb::make_iterator(nb::type<List>(), "list_iter", list.begin(), list.end());
    }, nb::keep_alive<0, 1>());
    list_cls.def("clear", [](List& list) {
        auto lock = list.write_lock();
        list.clear();
    });


    // structstore::Matrix
    auto matrix_cls = nb::class_<Matrix>(m, "StructStoreMatrix");
    py::ToPythonFn matrix_to_python_fn = [](const StructStoreField& field, py::ToPythonMode mode) -> nb::object {
        Matrix& m = field.get<Matrix>();
        if (mode == py::ToPythonMode::RECURSIVE) {
            return nb::cast(nb::ndarray<double, nb::c_contig, nb::numpy>(m.data(), m.ndim(), m.shape(), nb::handle()),
                            nb::rv_policy::copy);
        } else { // non-recursive convert
            return nb::cast(nb::ndarray<double, nb::c_contig, nb::numpy>(m.data(), m.ndim(), m.shape(), nb::handle()));
        }
    };
    py::FromPythonFn matrix_from_python_fn = [](FieldAccess access, const nanobind::handle& value) {
        if (py::copy_cast_from_python<Matrix>(access, value)) {
            return true;
        }
        if (nb::ndarray_check(value)) {
            auto array = nb::cast<nb::ndarray<const double, nb::c_contig>>(value);
            if (array.ndim() > Matrix::MAX_DIMS) {
                throw std::runtime_error("Incompatible buffer dimension!");
            }
            access.get<Matrix>().from(array.ndim(), (const size_t*) array.shape_ptr(), array.data());
            return true;
        }
        return false;
    };
    py::register_type<Matrix>(matrix_from_python_fn, matrix_to_python_fn,
                              py::default_to_python_cast_fn<Matrix>);
    py::register_complex_type_funcs<Matrix>(matrix_cls);
}

// required to make __iter__ method work
namespace nanobind::detail {
template<>
struct type_caster<structstore::StructStoreField> {
public:
    NB_TYPE_CASTER(structstore::StructStoreField, const_name("StructStoreField"))

    static handle from_cpp(structstore::StructStoreField& src, rv_policy, cleanup_list*) {
        return py::to_python_cast(src).release();
    }
};
}

#include "structstore/stst_containers.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_py.hpp"
#include "structstore/stst_shared.hpp"
#include "structstore/stst_structstore.hpp"
#include "structstore/stst_utils.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>

using namespace structstore;

namespace nb = nanobind;

NB_MODULE(MODULE_NAME, m) {
    // API types:

    py::SimpleNamespace = nb::module_::import_("types").attr("SimpleNamespace");

    nb::class_<ScopedFieldLock<false>>(m, "ScopedReadLock")
            .def("__enter__", [](ScopedFieldLock<false>&) {})
            .def(
                    "__exit__",
                    [](ScopedFieldLock<false>& con_man, nb::handle, nb::handle, nb::handle) {
                        con_man.unlock();
                    },
                    nb::arg().none(), nb::arg().none(), nb::arg().none());

    nb::class_<ScopedFieldLock<true>>(m, "ScopedWriteLock")
            .def("__enter__", [](ScopedFieldLock<true>&) {})
            .def(
                    "__exit__",
                    [](ScopedFieldLock<true>& con_man, nb::handle, nb::handle, nb::handle) {
                        con_man.unlock();
                    },
                    nb::arg().none(), nb::arg().none(), nb::arg().none());

    nb::enum_<CleanupMode>(m, "CleanupMode")
            .value("NEVER", NEVER)
            .value("IF_LAST", IF_LAST)
            .value("ALWAYS", ALWAYS)
            .export_values();

    nb::enum_<Log::Level>(m, "LogLevel")
            .value("DEBUG", Log::Level::DEBUG)
            .value("INFO", Log::Level::INFO)
            .value("WARN", Log::Level::WARN)
            .value("ERROR", Log::Level::ERROR)
            .export_values();
    m.def("set_log_level", [](Log::Level level) { Log::level = level; });

    // structstore::StructStore
    nb::class_<StructStore::Ref> cls = nb::class_<StructStore::Ref>{m, "StructStore"};
    cls.def("__init__",
            [](StructStore::Ref* store_ref) { StructStore::Ref::create_in_place(store_ref); });
    py::ToPythonFn structstore_to_python_fn = [](const FieldView& field_view,
                                                 py::ToPythonMode mode) -> nb::object {
        auto& store = field_view.get<StructStore>();
        return py::structstore_to_python(store, mode);
    };
    py::FromPythonFn structstore_from_python_fn = [](FieldAccess<true> access,
                                                     const nanobind::handle& value) {
        if (py::copy_cast_from_python<StructStore::Ref>(access, value)) { return true; }
        // todo: strict check if this is one of dict, SimpleNamespace, StructStore
        nb::dict dict;
        bool is_dict = false;
        if (nb::hasattr(value, "__dict__")) {
            dict = nb::dict(value.attr("__dict__"));
            is_dict = true;
        }
        if (nb::isinstance<nb::dict>(value)) {
            dict = nb::cast<nb::dict>(value);
            is_dict = true;
        }
        if (is_dict) {
            StructStore& store = access.get<StructStore>();
            STST_LOG_DEBUG() << "copying __dict__ to " << &store;
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
        return false;
    };
    py::register_type<StructStore::Ref>(structstore_from_python_fn, structstore_to_python_fn);
    py::register_structstore_funcs(cls);

    auto shcls = nb::class_<StructStoreShared>(m, "StructStoreShared");
    py::register_structstore_funcs(shcls);
    shcls.def(
            "__init__",
            [](StructStoreShared* s, const std::string& path, size_t size, bool reinit,
               bool use_file, CleanupMode cleanup) {
                new (s) StructStoreShared{path, size, reinit, use_file, cleanup};
            },
            nb::arg("path"), nb::arg("size") = 4096, nb::arg("reinit") = false,
            nb::arg("use_file") = false, nb::arg("cleanup") = IF_LAST);
    shcls.def(
            "__init__",
            [](StructStoreShared* s, int fd, bool init) { new (s) StructStoreShared{fd, init}; },
            nb::arg("fd"), nb::arg("init"));
    shcls.def("valid", &StructStoreShared::valid);
    shcls.def(
            "revalidate",
            [](StructStoreShared& shs, bool block) {
                bool res = false;
                do {
                    // necessary to get out of the loop on interrupting signal
                    if (PyErr_CheckSignals() != 0) { throw nb::python_error(); }
                    res = shs.revalidate(false);
                } while (res == false && block);
                return res;
            },
            nb::arg("block") = true);
    shcls.def("to_bytes", [](StructStoreShared& shs) {
        return nb::bytes((const char*) shs.addr(), shs.size());
    });
    shcls.def("from_bytes", [](StructStoreShared& shs, const nb::bytes& buffer) {
        shs.from_buffer((void*) buffer.c_str(), buffer.size());
    });
    shcls.def("close", &StructStoreShared::close);
    shcls.def_prop_ro("store", [](StructStoreShared& store) { return ref_wrap(*store); });

    // built-in field types:

    // basic types
    py::register_basic_type<int, nb::int_>();
    py::register_basic_type<double, nb::float_>();
    py::register_basic_type<bool, nb::bool_>();

    // structstore::string
    py::register_type<structstore::String>(
            [](FieldAccess<true> access, const nb::handle& value) {
                if (nb::isinstance<nb::str>(value)) {
                    access.get<structstore::String>() = nb::cast<std::string>(value).c_str();
                    return true;
                }
                return false;
            },
            [](const FieldView& field_view, py::ToPythonMode) -> nb::object {
                return nb::str(field_view.get<structstore::String>().c_str());
            },
            [](const FieldView& field_view) -> nb::object {
                return nb::str(field_view.get<structstore::String>().c_str());
            });

    // structstore::List
    auto list_cls = nb::class_<List::Ref>(m, "StructStoreList");
    list_cls.def("__init__", [](List::Ref* list_ref) { List::Ref::create_in_place(list_ref); });
    py::ToPythonFn list_to_python_fn = [](const FieldView& field_view,
                                          py::ToPythonMode mode) -> nb::object {
        auto& list = field_view.get<List>();
        auto ret = nb::list();
        for (Field& field_: list) {
            if (mode == py::ToPythonMode::RECURSIVE) {
                ret.append(py::to_python(field_.view(), py::ToPythonMode::RECURSIVE));
            } else { // non-recursive convert
                ret.append(py::to_python_cast(field_.view()));
            }
        }
        return ret;
    };
    py::FromPythonFn list_from_python_fn = [](FieldAccess<true> access,
                                              const nanobind::handle& value) {
        if (py::copy_cast_from_python<List::Ref>(access, value)) { return true; }
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
    py::register_type<List::Ref>(list_from_python_fn, list_to_python_fn);
    py::register_complex_type_funcs<List::Ref>(list_cls);
    list_cls.def("__len__", [](List::Ref& list_ref) {
        auto lock = list_ref->read_lock();
        return list_ref->size();
    });
    list_cls.def(
            "insert",
            [](List::Ref& list_ref, size_t index, nb::handle& value) {
                auto lock = list_ref->write_lock();
                FieldAccess access = list_ref->insert(index);
                py::from_python(access, value, std::to_string(index));
            },
            nb::arg("index"), nb::arg("value").none());
    list_cls.def("extend", [](List::Ref& list_ref, nb::iterable& value) {
        auto lock = list_ref->write_lock();
        for (const auto& val: value) {
            std::string field_name = std::to_string(list_ref->size());
            py::from_python(list_ref->push_back(), val, field_name);
        }
    });
    list_cls.def(
            "append",
            [](List::Ref& list_ref, nb::handle& value) {
                auto lock = list_ref->write_lock();
                FieldAccess access = list_ref->push_back();
                py::from_python(access, value, std::to_string(list_ref->size() - 1));
            },
            nb::arg("value").none());
    list_cls.def("pop", [](List::Ref& list_ref, size_t index) {
        auto lock = list_ref->write_lock();
        // todo: this returns a recursive copy, is this wanted?
        auto res =
                py::to_python((*list_ref)[index].get_field().view(), py::ToPythonMode::RECURSIVE);
        list_ref->erase(index);
        return res;
    });
    list_cls.def("__add__", [=](List::Ref& list_ref, nb::list& value) {
        auto lock = list_ref->read_lock();
        // todo: this returns a recursive copy, is this wanted?
        return py::to_python(FieldView{*list_ref}, py::ToPythonMode::RECURSIVE) + value;
    });
    list_cls.def("__iadd__", [=](List::Ref& list_ref, nb::list& value) {
        auto lock = list_ref->write_lock();
        for (const auto& val: value) {
            py::from_python(list_ref->push_back(), val, std::to_string(list_ref->size() - 1));
        }
        return py::to_python_cast(FieldView{*list_ref});
    });
    list_cls.def(
            "__setitem__",
            [](List::Ref& list_ref, size_t index, nb::handle& value) {
                auto lock = list_ref->write_lock();
                py::from_python((*list_ref)[index], value, std::to_string(index));
            },
            nb::arg("index"), nb::arg("value").none());
    list_cls.def("__getitem__", [](List::Ref& list_ref, size_t index) {
        auto lock = list_ref->read_lock();
        STST_LOG_DEBUG() << "getting list item of type "
                         << typing::get_type((*list_ref)[index].get_field().get_type_hash()).name;
        return py::to_python_cast((*list_ref)[index].get_field().view());
    });
    list_cls.def("__delitem__", [](List::Ref& list_ref, size_t index) {
        auto lock = list_ref->write_lock();
        list_ref->erase(index);
    });
    list_cls.def(
            "__iter__",
            [](List::Ref& list_ref) {
                auto lock = list_ref->read_lock();
                return nb::make_iterator(nb::type<List>(), "list_iter", list_ref->begin(),
                                         list_ref->end());
            },
            nb::keep_alive<0, 1>());
    list_cls.def("clear", [](List::Ref& list_ref) {
        auto lock = list_ref->write_lock();
        list_ref->clear();
    });
    list_cls.def("check", [](List::Ref& list_ref) {
        STST_LOG_DEBUG() << "checking from python ...";
        list_ref.check();
    });


    // structstore::Matrix
    auto matrix_cls = nb::class_<Matrix::Ref>(m, "StructStoreMatrix");
    py::ToPythonFn matrix_to_python_fn = [](const FieldView& field_view,
                                            py::ToPythonMode mode) -> nb::object {
        Matrix& m = field_view.get<Matrix>();
        if (mode == py::ToPythonMode::RECURSIVE) {
            return nb::cast(nb::ndarray<double, nb::c_contig, nb::numpy>(m.data(), m.ndim(), m.shape(), nb::handle()),
                            nb::rv_policy::copy);
        } else { // non-recursive convert
            return nb::cast(nb::ndarray<double, nb::c_contig, nb::numpy>(m.data(), m.ndim(),
                                                                         m.shape(), nb::handle()),
                            nb::rv_policy::reference);
        }
    };
    py::FromPythonFn matrix_from_python_fn = [](FieldAccess<true> access,
                                                const nanobind::handle& value) {
        if (py::copy_cast_from_python<Matrix::Ref>(access, value)) { return true; }
        if (nb::ndarray_check(value)) {
            auto array = nb::cast<nb::ndarray<const double, nb::c_contig>>(value);
            if (array.ndim() > Matrix::MAX_DIMS) {
                throw std::runtime_error("Incompatible buffer dimension!");
            }
            access.get<Matrix>().from(array.ndim(), (const size_t*) array.shape_ptr(),
                                      array.data());
            return true;
        }
        return false;
    };
    py::register_type<Matrix::Ref>(matrix_from_python_fn, matrix_to_python_fn);
    py::register_complex_type_funcs<Matrix::Ref>(matrix_cls);
}

// required to make __iter__ method work
namespace nanobind::detail {
template<>
struct type_caster<structstore::Field> {
public:
    NB_TYPE_CASTER(structstore::Field, const_name("Field"))

    static handle from_cpp(structstore::Field& src, rv_policy, cleanup_list*) {
        return py::to_python_cast(src.view()).release();
    }
};
}

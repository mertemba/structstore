#ifndef STST_PY_HPP
#define STST_PY_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_shared.hpp"
#include "structstore/stst_structstore.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <functional>
#include <type_traits>
#include <unordered_map>

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

// make customized STL containers opaque to nanobind
namespace nanobind::detail {
template<class T>
class type_caster<structstore::vector<T>>
    : public type_caster_base<structstore::vector<T>> {};

template<class K, class T>
class type_caster<structstore::unordered_map<K, T>>
    : public type_caster_base<structstore::unordered_map<K, T>> {};
} // namespace nanobind::detail

namespace structstore {

namespace nb = nanobind;

class py {
public:
    enum class ToPythonMode {
        NON_RECURSIVE,
        RECURSIVE,
    };

    using FromPythonFn = std::function<bool(FieldAccess, const nb::handle&)>;
    using ToPythonFn = std::function<nb::object(const Field&, ToPythonMode mode)>;
    using ToPythonCastFn = std::function<nb::object(const Field&)>;

private:
    struct __attribute__((__visibility__("default"))) PyType {
        const FromPythonFn from_python_fn;
        const ToPythonFn to_python_fn;
        const ToPythonCastFn to_python_cast_fn;
    };

    static std::unordered_map<uint64_t, const PyType>& get_py_types();

    static const PyType& get_py_type(uint64_t type_hash);

    static StructStore& get_store(StructStore& store) { return store; }
    static StructStore& get_store(StructStoreShared& store) { return *store; }
    template<typename T>
    static StructStore& get_store(Struct<T>& s) {
        return s.store;
    }

    static nb::object get_field(StructStore& store, const std::string& name);

    static void set_field(StructStore& store, const std::string& name, const nb::handle& value);

    static ScopedLock lock(StructStore& store);

    static nb::object __repr__(StructStore& store);

    static nb::object copy(StructStore& store);

    static nb::object deepcopy(StructStore& store);

    static nb::object __copy__(StructStore& store);

    static nb::object __deepcopy__(StructStore& store, nb::handle&);

    static void clear(StructStore& store);

public:
    template<typename T, typename T_py>
    static bool default_from_python_fn(FieldAccess access, const nb::handle& value) {
        if (nb::isinstance<T>(value)) {
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name
                             << " succeeded";
            const T& t = nb::cast<T>(value, false);
            access.get<T>() = t;
            return true;
        }
        STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
        return false;
    };

    template<typename T, typename T_py>
    static nb::object default_to_python_fn(const Field& field, ToPythonMode) {
        return T_py(field.get<T>());
    };

    template<typename T>
    static nb::object default_to_python_cast_fn(const Field& field) {
        return nb::cast(field.get<T>(), nb::rv_policy::reference);
    };

    template<typename T>
    static void register_type(FromPythonFn from_python_fn, ToPythonFn to_python_fn,
                              ToPythonCastFn to_python_cast_fn = default_to_python_cast_fn<T>) {
        PyType py_type{from_python_fn, to_python_fn, to_python_cast_fn};
        const uint64_t type_hash = typing::get_type_hash<T>();
        STST_LOG_DEBUG() << "registering Python type '" << typing::get_type<T>().name
                         << "' with hash '" << type_hash << "'";
        auto ret = get_py_types().insert({type_hash, py_type});
        if (!ret.second) {
            throw typing::already_registered_type_error(type_hash);
        }
    }

    static nb::object structstore_to_python(StructStore& store, py::ToPythonMode mode);

    template<typename T>
    static void register_struct_ptr_type() {
        static_assert(!std::is_pointer_v<T>);
        static_assert(std::is_class_v<T>);
        auto from_python_fn = [](FieldAccess access, const nb::handle& value) {
            if (access.get_type_hash() == typing::get_type_hash<T*>() && nb::isinstance<T>(value)) {
                STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name
                                 << " succeeded";
                T& t = nb::cast<T&>(value, false);
                access.get<T*>() = &t;
                return true;
            }
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
            return false;
        };
        py::ToPythonFn to_python_fn = [](const Field& field, py::ToPythonMode) {
            T* t_ptr = field.get<T*>();
            if (t_ptr == nullptr) {
                return nb::none();
            }
            return nb::cast(*t_ptr, nb::rv_policy::reference);
        };
        py::ToPythonCastFn to_python_cast_fn = [](const Field& field) {
            T* t_ptr = field.get<T*>();
            if (t_ptr == nullptr) {
                return nb::none();
            }
            return nb::cast(*t_ptr, nb::rv_policy::reference);
        };
        register_type<T*>(from_python_fn, to_python_fn, to_python_cast_fn);
    }

    template<typename T>
    static void register_struct_type(nb::class_<T>& cls) {
        static_assert(std::is_base_of_v<Struct<T>, T>);
        static_assert(std::is_same_v<T, std::remove_cv_t<T>>);
        py::ToPythonFn to_python_fn = [](const Field& field, py::ToPythonMode mode) {
            auto& store = get_store(field.get<T>());
            return structstore_to_python(store, mode);
        };
        register_type<T>(default_from_python_fn<T, nb::class_<T>>, to_python_fn);
        register_struct_ptr_type<T>();
        register_structstore_funcs(cls);
    }

    static const FromPythonFn& get_from_python_fn(uint64_t type_hash) {
        return get_py_type(type_hash).from_python_fn;
    }

    static const ToPythonFn& get_to_python_fn(uint64_t type_hash) {
        return get_py_type(type_hash).to_python_fn;
    }

    static const ToPythonCastFn& get_to_python_cast_fn(uint64_t type_hash) {
        return get_py_type(type_hash).to_python_cast_fn;
    }

    template<typename T>
    static void register_basic_ptr_type() {
        static_assert(!std::is_pointer_v<T>);
        static_assert(!std::is_class_v<T>);
        auto from_python_fn = [](FieldAccess access, const nb::handle& value) {
            if (access.get_type_hash() == typing::get_type_hash<T*>() && nb::isinstance<T>(value)) {
                STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name
                                 << " succeeded";
                T t = nb::cast<T>(value, false);
                *access.get<T*>() = t;
                return true;
            }
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
            return false;
        };
        py::ToPythonFn to_python_fn = [](const Field& field, py::ToPythonMode) {
            T* t_ptr = field.get<T*>();
            if (t_ptr == nullptr) {
                return nb::none();
            }
            return nb::cast(*t_ptr);
        };
        py::ToPythonCastFn to_python_cast_fn = [](const Field& field) {
            T* t_ptr = field.get<T*>();
            if (t_ptr == nullptr) {
                return nb::none();
            }
            return nb::cast(*t_ptr);
        };
        register_type<T*>(from_python_fn, to_python_fn, to_python_cast_fn);
    }

    template<typename T, typename T_py>
    static void register_basic_type() {
        register_type<T>(default_from_python_fn<T, T_py>, default_to_python_fn<T, T_py>,
                         default_to_python_cast_fn<T>);
        register_basic_ptr_type<T>();
    }

    template<typename T>
    static void register_complex_type_funcs(nb::class_<T>& cls) {
        static_assert(!std::is_pointer_v<T>);
        cls.def("to_yaml", [](T& t) { return YAML::Dump(FieldView { t } -> to_yaml()); });
        cls.def("__repr__", [](T& t) {
            return (std::ostringstream() << *FieldView{t}).str();
        });
        cls.def("copy", [](T& t) {
            return to_python(*FieldView{t}, ToPythonMode::NON_RECURSIVE);
        });
        cls.def("deepcopy", [](T& t) {
            return to_python(*FieldView{t}, ToPythonMode::RECURSIVE);
        });
        cls.def("__copy__", [](T& t) {
            return to_python(*FieldView{t}, ToPythonMode::NON_RECURSIVE);
        });
        cls.def("__deepcopy__", [](T& t, nb::handle&) {
            return to_python(*FieldView{t}, ToPythonMode::RECURSIVE);
        });
        cls.def("__eq__", [](T& t, nb::handle& other) {
            if (const T* other_ = try_cast<T>(other)) {
                return t == *other_;
            }
            return false;
        });
    }

    template<typename T>
    static T* try_cast(const nb::handle& value) {
        try {
            return &nb::cast<T&>(value, false);
        } catch (const nb::cast_error&) {
            return nullptr;
        }
    }

    template<typename T>
    static bool copy_cast_from_python(FieldAccess access, const nb::handle& value) {
        T* value_cpp = try_cast<T>(value);
        if (value_cpp == nullptr) {
            return false;
        }
        T& field_cpp = access.get<T>();
        STST_LOG_DEBUG() << "at type " << typing::get_type<T>().name;
        if (value_cpp == &field_cpp) {
            STST_LOG_DEBUG() << "copying to itself";
            return true;
        }
        STST_LOG_DEBUG() << "copying " << typing::get_type<T>().name << " from " << value_cpp << " to " << &field_cpp;
        field_cpp = *value_cpp;
        return true;
    }

    template<typename T>
    static void register_structstore_funcs(nb::class_<T>& cls) {
        register_complex_type_funcs<T>(cls);

        cls.def("__getstate__", [](StructStore& store) {
            nb::object dict = _to_python(store, py::ToPythonMode::RECURSIVE);
            return dict;
        });

        if constexpr (std::is_same_v<T, StructStore>) {
            cls.def("__setstate__", [](StructStore& store, nb::handle value) {
                new (&store) StructStore(static_alloc);
                _from_python(store, store.mm_alloc, value, "<root>");
            });
        } else {
            cls.def("__setstate__", [](T&, nb::handle) {
                throw std::runtime_error("cannot unpickle type " + std::string(typeid(T).name()));
            });
        }

        cls.def(
                "__getattr__",
                [](T& t, const std::string& name) {
                    auto& store = get_store(t);
                    return get_field(store, name);
                },
                nb::arg("name"), nb::rv_policy::reference_internal);

        cls.def(
                "__setattr__",
                [](T& t, const std::string& name, const nb::handle& value) {
                    auto& store = get_store(t);
                    return set_field(store, name, value);
                },
                nb::arg("name"), nb::arg("value").none());

        cls.def(
                "__delattr__",
                [](T& t, const std::string& name) {
                    auto& store = get_store(t);
                    return store.remove(name.c_str());
                },
                nb::arg("name"));

        cls.def(
                "__getitem__",
                [](T& t, const std::string& name) {
                    auto& store = get_store(t);
                    return get_field(store, name);
                },
                nb::arg("name"), nb::rv_policy::reference_internal);

        cls.def(
                "__setitem__",
                [](T& t, const std::string& name, const nb::handle& value) {
                    auto& store = get_store(t);
                    return set_field(store, name, value);
                },
                nb::arg("name"), nb::arg("value").none());

        cls.def(
                "__delitem__",
                [](T& t, const std::string& name) {
                    auto& store = get_store(t);
                    return store.remove(name.c_str());
                },
                nb::arg("name"));

        cls.def("empty", [](T& t) {
            auto& store = get_store(t);
            return store.empty();
        });

        cls.def(
                "lock",
                [](T& t) {
                    auto& store = get_store(t);
                    return lock(store);
                },
                nb::rv_policy::move);

        cls.def("clear", [](T& t) {
            auto& store = get_store(t);
            return clear(store);
        });

        cls.def("check", [](T& t) {
            STST_LOG_DEBUG() << "checking from python ...";
            auto& store = get_store(t);
            return store.check();
        });
    }

    static nb::object to_python(const Field& field, ToPythonMode mode);

    static nb::object to_python_cast(const Field& field);

    static void from_python(FieldAccess access, const nb::handle& value, const std::string& field_name);

protected:
    template<typename T>
    static void _from_python(T& t, MiniMalloc& mm_alloc,
                             const nb::handle& value,
                             const std::string& field_name) {
        from_python(*AccessView{t, mm_alloc}, value, field_name);
    }

    template<typename T>
    static nb::object _to_python(T& t, ToPythonMode mode) {
        return to_python(*FieldView{t}, mode);
    }
};

} // namespace structstore

#endif

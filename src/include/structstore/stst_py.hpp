#ifndef STST_PY_HPP
#define STST_PY_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_containers.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_structstore.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <functional>
#include <unordered_map>

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>

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
private:
    static nb::object get_field(StructStore& store, const std::string& name);

    static void set_field(StructStore& store, const std::string& name, const nb::object& value);

    static ScopedLock lock(StructStore& store);

    static nb::object __repr__(StructStore& store);

    static nb::object copy(StructStore& store);

    static nb::object deepcopy(StructStore& store);

    static nb::object __copy__(StructStore& store);

    static nb::object __deepcopy__(StructStore& store, nb::handle&);

    static void clear(StructStore& store);

public:
    enum class ToPythonMode {
        NON_RECURSIVE,
        RECURSIVE,
    };

    using FromPythonFn = std::function<bool(FieldAccess, const nb::handle&)>;
    using ToPythonFn = std::function<nb::object(const StructStoreField&, ToPythonMode mode)>;
    using ToPythonCastFn = std::function<nb::object(const StructStoreField&)>;

    static std::unordered_map<uint64_t, FromPythonFn>& get_from_python_fns();

    static std::unordered_map<uint64_t, ToPythonFn>& get_to_python_fns();

    static std::unordered_map<uint64_t, ToPythonCastFn>& get_to_python_cast_fns();

    template<typename T>
    static void register_from_python_fn(const FromPythonFn& from_python_fn) {
        uint64_t type_hash = typing::get_type_hash<T>();
        bool success = get_from_python_fns().insert({type_hash, from_python_fn}).second;
        if (!success) {
            throw typing::already_registered_type_error(type_hash);
        }
    }

protected:
    template<typename T>
    static void register_to_python_cast_fn() {
        uint64_t type_hash = typing::get_type_hash<T>();
        auto to_python_cast_fn = [](const StructStoreField& field) -> nb::object {
            return nb::cast(field.get<T>(), nb::rv_policy::reference);
        };
        bool success = get_to_python_cast_fns().insert({type_hash, to_python_cast_fn}).second;
        if (!success) {
            throw typing::already_registered_type_error(type_hash);
        }
    }

public:
    template<typename T>
    static void register_to_python_fn(const ToPythonFn& to_python_fn) {
        register_to_python_cast_fn<T>();
        uint64_t type_hash = typing::get_type_hash<T>();
        bool success = get_to_python_fns().insert({type_hash, to_python_fn}).second;
        if (!success) {
            throw typing::already_registered_type_error(type_hash);
        }
    }

    template<typename T_cpp, typename T_py>
    static void register_basic_type() {
        static_assert(!std::is_pointer<T_cpp>::value);
        register_to_python_fn<T_cpp>([](const StructStoreField& field, ToPythonMode) -> nb::object {
            if constexpr (std::is_same<T_cpp, structstore::string>::value) {
                return T_py(field.get<T_cpp>().c_str());
            } else {
                return T_py(field.get<T_cpp>());
            }
        });
        register_from_python_fn<T_cpp>([](FieldAccess access, const nb::handle& value) {
            if (nb::isinstance<T_py>(value)) {
                if constexpr (std::is_same<T_cpp, structstore::string>::value) {
                    access.get<T_cpp>() = nb::cast<std::string>(value);
                } else {
                    access.get<T_cpp>() = nb::cast<T_cpp>(value);
                }
                return true;
            }
            return false;
        });
    }

    template<typename T>
    static void register_complex_type_funcs(nb::class_<T>& cls) {
        static_assert(!std::is_pointer<T>::value);
        cls.def("to_yaml", [](T& t) {
            return YAML::Dump(to_yaml(*FieldView{t}));
        });
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
    static nb::class_<T> register_complex_type(nb::handle scope) {
        auto cls = nb::class_<T>(scope, typing::get_type_name<T>().c_str());
        register_complex_type_funcs(cls);
        return cls;
    }

    template<typename T_cpp>
    static void register_ptr_type(const nb::handle& T_ptr_py, const nb::handle& T_py) {
        static_assert(std::is_pointer<T_cpp>::value);
        // todo: register to_python_fn
        register_from_python_fn<T_cpp>([T_ptr_py, T_py](FieldAccess access, const nb::handle& value) {
            if (value.type().equal(T_ptr_py) || (!access.get_field().empty() && value.type().equal(T_py))) {
                access.get<T_cpp>() = nb::cast<T_cpp>(value);
                return true;
            }
            return false;
        });
    }

    template<typename T_cpp>
    static T_cpp* try_cast(const nb::handle& value) {
        try {
            return &nb::cast<T_cpp&>(value, false);
        } catch (const nb::cast_error&) {
            return nullptr;
        }
    }

    template<typename T_cpp>
    static bool copy_cast_from_python(FieldAccess access, const nb::handle& value) {
        T_cpp* value_cpp = try_cast<T_cpp>(value);
        if (value_cpp == nullptr) {
            return false;
        }
        T_cpp& field_cpp = access.get<T_cpp>();
        STST_LOG_DEBUG() << "at type " << typing::get_type_name<T_cpp>();
        if (value_cpp == &field_cpp) {
            STST_LOG_DEBUG() << "copying to itself";
            return true;
        }
        STST_LOG_DEBUG() << "copying " << typing::get_type_name<T_cpp>() << " from " << value_cpp << " to " << &field_cpp;
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

        if constexpr (std::is_same<T, StructStore>::value) {
            cls.def("__setstate__", [](StructStore& store, nb::handle value) {
                new (&store) StructStore(static_alloc);
                _from_python(store, store.mm_alloc, value, "<root>");
            });
        } else {
            cls.def("__setstate__", [](T&, nb::handle) {
                throw std::runtime_error("cannot unpickle type " + typing::get_type_name<T>());
            });
        }

        cls.def("__getattr__", [](T& t, const std::string& name) {
            auto& store = t.get_store();
            return get_field(store, name); }, nb::arg("name"), nb::rv_policy::reference_internal);

        cls.def("__setattr__", [](T& t, const std::string& name, const nb::object& value) {
            auto& store = t.get_store();
            return set_field(store, name, value); }, nb::arg("name"), nb::arg("value").none());

        cls.def("__getitem__", [](T& t, const std::string& name) {
            auto& store = t.get_store();
            return get_field(store, name); }, nb::arg("name"), nb::rv_policy::reference_internal);

        cls.def("__setitem__", [](T& t, const std::string& name, const nb::object& value) {
            auto& store = t.get_store();
            return set_field(store, name, value); }, nb::arg("name"), nb::arg("value").none());

        cls.def("lock", [](T& t) {
            auto& store = t.get_store();
            return lock(store); }, nb::rv_policy::move);

        cls.def("clear", [](T& t) {
            auto& store = t.get_store();
            return clear(store);
        });

        cls.def("check", [](T& t) {
            STST_LOG_DEBUG() << "checking from python ...";
            auto& store = t.get_store();
            return store.check();
        });
    }

    static nb::object to_python(const StructStoreField& field, ToPythonMode mode);

    static nb::object to_python_cast(const StructStoreField& field);

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

public:
    template<typename T>
    static nb::class_<T> register_struct_type(nb::module_& m, const std::string& name) {
        static_assert(std::is_base_of<Struct, T>::value,
                      "template parameter is not derived from structstore::Struct");
        typing::register_type<T>(name);
        typing::register_mm_alloc_constructor<T>();
        typing::register_default_destructor<T>();
        typing::register_default_serializer_text<T>();
        typing::register_check<T>([](MiniMalloc&, const T* t) {
            get_store(*t).check();
        });

        auto nb_cls = nb::class_<T>(m, name.c_str());
        // nb_cls.def(nb::init());
        nb_cls.def("__init__", [=](T* t) { try_with_info("in constructor of " << name << ": ", new (t) T();); });
        py::register_complex_type<T>(nb_cls);
        py::register_structstore_funcs(nb_cls);
        return nb_cls;
    }
};

} // namespace structstore

#endif

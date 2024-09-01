#ifndef STST_BINDINGS_HPP
#define STST_BINDINGS_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_containers.hpp"
#include "structstore/stst_stl_types.hpp"
#include "structstore/stst_structstore.hpp"
#include "structstore/stst_typing.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>

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

nb::object to_object(const StructStoreField& field, bool recursive);

nb::object to_object(const StructStore& store, bool recursive);

nb::object to_object(const List& list, bool recursive);

void from_object(FieldAccess access, const nb::handle& value, const std::string& field_name);

class bindings {
private:
    static nb::object __slots__(StructStore& store);

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
    static nb::object SimpleNamespace;

    using FromPythonFn = std::function<bool(FieldAccess, const nb::handle&)>;
    using ToPythonFn = std::function<nb::object(const StructStoreField&, bool recursive)>;

    static std::unordered_map<uint64_t, FromPythonFn>& get_from_python_fns();

    static std::unordered_map<uint64_t, ToPythonFn>& get_to_python_fns();

    template<typename T>
    static void register_from_python_fn(const FromPythonFn& from_python_fn) {
        uint64_t type_hash = typing::get_type_hash<T>();
        bool success = get_from_python_fns().insert({type_hash, from_python_fn}).second;
        if (!success) {
            throw typing::already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_to_python_fn(const ToPythonFn& to_python_fn) {
        uint64_t type_hash = typing::get_type_hash<T>();
        bool success = get_to_python_fns().insert({type_hash, to_python_fn}).second;
        if (!success) {
            throw typing::already_registered_type_error(type_hash);
        }
    }

    template<typename T_cpp>
    static void register_basic_to_python() {
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nb::object {
            return nb::cast(field.get<T_cpp>(), nb::rv_policy::reference);
        });
    }

    template<typename T_cpp, typename T_py>
    static void register_basic_bindings() {
        static_assert(!std::is_pointer<T_cpp>::value);
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nb::object {
            return T_py(field.get<T_cpp>());
        });
        register_from_python_fn<T_cpp>([](FieldAccess access, const nb::handle& value) {
            if (nb::isinstance<T_py>(value)) {
                access.get<T_cpp>() = nb::cast<T_cpp>(value);
                return true;
            }
            return false;
        });
    }

    template<typename T_cpp>
    static void register_basic_bindings(const nb::handle& T_py) {
        static_assert(!std::is_pointer<T_cpp>::value);
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nb::object {
            return nb::cast(field.get<T_cpp>(), nb::rv_policy::reference);
        });
        register_from_python_fn<T_cpp>([T_py](FieldAccess access, const nb::handle& value) {
            if (value.type().equal(T_py)) {
                access.get<T_cpp>() = nb::cast<T_cpp&>(value);
                return true;
            }
            return false;
        });
    }

    template<typename T_cpp>
    static void register_ptr_bindings(const nb::handle& T_ptr_py, const nb::handle& T_py) {
        static_assert(std::is_pointer<T_cpp>::value);
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nb::object {
            return nb::cast(field.get<T_cpp>(), nb::rv_policy::reference);
        });
        register_from_python_fn<T_cpp>([T_ptr_py, T_py](FieldAccess access, const nb::handle& value) {
            if (value.type().equal(T_ptr_py) || (!access.get_field().empty() && value.type().equal(T_py))) {
                access.get<T_cpp>() = nb::cast<T_cpp>(value);
                return true;
            }
            return false;
        });
    }

    template<typename T_cpp>
    static void register_object_to_python() {
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nb::object {
            if (recursive) {
                return to_object(field.get<T_cpp>(), true);
            } else {
                return nb::cast(field.get<T_cpp>(), nb::rv_policy::reference);
            }
        });
    }

    template<typename T_cpp>
    static bool object_from_python(FieldAccess access, const nb::handle& value) {
        try {
            T_cpp& value_cpp = nb::cast<T_cpp&>(value, false);
            T_cpp& field_cpp = access.get<T_cpp>();
            if (&value_cpp == &field_cpp) {
                return true;
            }
            field_cpp = value_cpp;
            return true;
        } catch (const nb::cast_error&) {
        }
        return false;
    }

    template<typename T_cpp>
    static void register_object_from_python() {
        register_from_python_fn<T_cpp>(object_from_python<T_cpp>);
    }

    template<typename T>
    static void register_structstore_bindings(nb::class_<T>& cls) {
        cls.def_prop_ro("__slots__", [](T& t) {
            auto& store = t.get_store();
            return __slots__(store);
        });

        cls.def("__getattr__", [](T& t, const std::string& name) {
            auto& store = t.get_store();
            return get_field(store, name);
        }, nb::arg("name"), nb::rv_policy::reference_internal);

        cls.def("__setattr__", [](T& t, const std::string& name, const nb::object& value) {
            auto& store = t.get_store();
            return set_field(store, name, value);
        }, nb::arg("name"), nb::arg("value").none());

        cls.def("__getitem__", [](T& t, const std::string& name) {
            auto& store = t.get_store();
            return get_field(store, name);
        }, nb::arg("name"), nb::rv_policy::reference_internal);

        cls.def("__setitem__", [](T& t, const std::string& name, const nb::object& value) {
            auto& store = t.get_store();
            return set_field(store, name, value);
        }, nb::arg("name"), nb::arg("value").none());

        cls.def("lock", [](T& t) {
            auto& store = t.get_store();
            return lock(store);
        }, nb::rv_policy::move);

        cls.def("to_yaml", [](T& t) {
            auto& store = t.get_store();
            auto lock = store.read_lock();
            return YAML::Dump(to_yaml(store));
        });

        cls.def("__repr__", [](T& t) {
            auto& store = t.get_store();
            return __repr__(store);
        });

        cls.def("copy", [](T& t) {
            auto& store = t.get_store();
            return copy(store);
        });

        cls.def("deepcopy", [](T& t) {
            auto& store = t.get_store();
            return deepcopy(store);
        });

        cls.def("__copy__", [](T& t) {
            auto& store = t.get_store();
            return copy(store);
        });

        cls.def("__deepcopy__", [](T& t, nb::handle&) {
            auto& store = t.get_store();
            return deepcopy(store);
        });

        cls.def("clear", [](T& t) {
            auto& store = t.get_store();
            return clear(store);
        });

        cls.def("check", [](T& t) {
            auto& store = t.get_store();
            return store.check();
        });
    }
};

template<typename T>
nb::class_<T> register_struct_type_and_bindings(nb::module_& m, const std::string& name) {
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
    nb_cls.def(nb::init());
    bindings::register_basic_bindings<T>(nb_cls);
    bindings::register_structstore_bindings(nb_cls);
    return nb_cls;
}

}

#endif

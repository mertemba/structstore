#ifndef STST_BINDINGS_HPP
#define STST_BINDINGS_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_structstore.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>

#include <nanobind/nanobind.h>

namespace structstore {

class bindings {
public:

    using FromPythonFn = std::function<bool(FieldAccess, const nanobind::handle&)>;
    using ToPythonFn = std::function<nanobind::object(const StructStoreField&, bool recursive)>;

    static std::vector<FromPythonFn>& get_from_python_fns();

    static std::unordered_map<uint64_t, ToPythonFn>& get_to_python_fns();

    static void register_from_python_fn(const FromPythonFn& from_python_fn);

    template<typename T>
    static bool register_to_python_fn(const ToPythonFn& to_python_fn) {
        bool success = get_to_python_fns().insert(
                {typing::get_type_hash<T>(), to_python_fn}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
        return true;
    }

    template<typename T_cpp>
    static void register_basic_to_python() {
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nanobind::object {
            return nanobind::cast(field.get<T_cpp>());
        });
    }

    template<typename T_cpp, typename T_py>
    static void register_basic_binding() {
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nanobind::object {
            return T_py(field.get<T_cpp>());
        });
        register_from_python_fn([](FieldAccess access, const nanobind::handle& value) {
            if (nanobind::isinstance<T_py>(value)) {
                access.set_type<T_cpp>();
                access.get<T_cpp>() = nanobind::cast<T_cpp>(value);
                return true;
            }
            return false;
        });
    }

    template<typename T_cpp>
    static void register_basic_binding(const nanobind::handle& T_py) {
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nanobind::object {
            return nanobind::cast(field.get<T_cpp>());
        });
        register_from_python_fn([T_py](FieldAccess access, const nanobind::handle& value) {
            if (value.type().equal(T_py)) {
                access.set_type<T_cpp>();
                access.get<T_cpp>() = nanobind::cast<T_cpp>(value);
                return true;
            }
            return false;
        });
    }

    template<typename T_cpp>
    static void register_object_to_python() {
        register_to_python_fn<T_cpp>([](const StructStoreField& field, bool recursive) -> nanobind::object {
            if (recursive) {
                return to_object(field.get<T_cpp>(), true);
            } else {
                return nanobind::cast(field.get<T_cpp>(), nanobind::rv_policy::reference);
            }
        });
    }

    template<typename T_cpp>
    static void register_object_from_python() {
        register_from_python_fn([](FieldAccess access, const nanobind::handle& value) {
            uint64_t type_hash = typing::get_type_hash<T_cpp>();
            if (access.get_field().empty() || access.get_type_hash() == type_hash) {
                try {
                    T_cpp& value_cpp = nanobind::cast<T_cpp&>(value, false);
                    access.set_type<T_cpp>();
                    T_cpp& field_cpp = access.get<T_cpp>();
                    if (&value_cpp == &field_cpp) {
                        return true;
                    }
                    access.get<T_cpp>() = value_cpp;
                    return true;
                } catch (const nanobind::cast_error&) {
                }
            }
            return false;
        });
    }
};

}

#endif

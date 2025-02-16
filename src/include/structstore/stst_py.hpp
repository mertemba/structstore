#ifndef STST_PY_HPP
#define STST_PY_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_fieldmap.hpp"
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
class type_caster<structstore::shr_vector<T>>
    : public type_caster_base<structstore::shr_vector<T>> {};

template<class K, class T>
class type_caster<structstore::shr_unordered_map<K, T>>
    : public type_caster_base<structstore::shr_unordered_map<K, T>> {};
} // namespace nanobind::detail

namespace structstore {

namespace nb = nanobind;

class py {
public:
    enum class ToPythonMode {
        NON_RECURSIVE,
        RECURSIVE,
    };

    using FromPythonFn = std::function<bool(FieldAccess<true>, const nb::handle&)>;
    using ToPythonFn = std::function<nb::object(const FieldView&, ToPythonMode mode)>;
    using ToPythonCastFn = std::function<nb::object(const FieldView&)>;

    __attribute__((__visibility__("default"))) static nb::object SimpleNamespace;

private:
    struct __attribute__((__visibility__("default"))) PyType {
        const FromPythonFn from_python_fn;
        const ToPythonFn to_python_fn;
        const ToPythonCastFn to_python_cast_fn;
    };

    static std::unordered_map<type_hash_t, const PyType>& get_py_types();

    static const PyType& get_py_type(type_hash_t type_hash);

    static nb::object get_field(const FieldMapBase& field_map, const std::string& name);

    static void set_field(FieldMap<false>& field_map, const std::string& name,
                          const nb::handle& value, const FieldTypeBase& parent_field);
    static void set_field(FieldMap<true>& field_map, const std::string& name,
                          const nb::handle& value, const FieldTypeBase& parent_field);

public:
    template<typename W, typename W_py>
    static bool default_from_python_fn(FieldAccess<true> access, const nb::handle& value) {
        using T = unwrap_type_t<W>;
        if (nb::isinstance<T>(value)) {
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name
                             << " succeeded";
            const W& w = nb::cast<W>(value, false);
            access.get<T>() = unwrap<W>(w);
            return true;
        }
        STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
        return false;
    }

    template<typename W, typename W_py>
    static nb::object default_to_python_fn(const FieldView& field_view, ToPythonMode) {
        using T = unwrap_type_t<W>;
        return W_py(ref_wrap(field_view.get<T>()));
    }

    template<typename W>
    static nb::object default_to_python_cast_fn(const FieldView& field_view) {
        using T = unwrap_type_t<W>;
        return nb::cast(ref_wrap(field_view.get<T>()), nb::rv_policy::reference);
    }

    template<typename W>
    static void register_type(FromPythonFn from_python_fn, ToPythonFn to_python_fn,
                              ToPythonCastFn to_python_cast_fn = default_to_python_cast_fn<W>) {
        static_assert(std::is_same_v<W, std::remove_const_t<W>>);
        static_assert(std::is_same_v<W, std::remove_reference_t<W>>);
        // check that a wrapper type (or String) is provided:
        static_assert(std::is_same_v<std::remove_reference_t<wrap_type_w<W>>, W>);
        using T = unwrap_type_t<W>;
        static_assert(typing::is_field_type<unwrap_type_t<W>>);

        PyType py_type{from_python_fn, to_python_fn, to_python_cast_fn};
        const type_hash_t type_hash = typing::get_type_hash<T>();
        STST_LOG_DEBUG() << "registering Python type '" << typing::get_type<T>().name
                         << "' with hash '" << type_hash << "'";
        auto ret = get_py_types().insert({type_hash, py_type});
        if (!ret.second) { throw typing::already_registered_type_error(type_hash); }
    }

    static nb::object field_map_to_python(const FieldMapBase& field_map, py::ToPythonMode mode);

    static void field_map_from_python(FieldMap<false>& field_map, const nb::dict& dict,
                                      const FieldTypeBase& parent_field);

    inline static nb::object structstore_to_python(const StructStore& store,
                                                   py::ToPythonMode mode) {
        return field_map_to_python(store.field_map, mode);
    }

    template<typename W>
    static void register_struct_ptr_type() {
        using T = unwrap_type_t<W>;
        static_assert(!std::is_pointer_v<T>);
        static_assert(std::is_class_v<T>);
        auto from_python_fn = [](FieldAccess<true> access, const nb::handle& value) {
            if (access.get_type_hash() == typing::get_type_hash<OffsetPtr<T>>() &&
                nb::isinstance<W>(value)) {
                STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name
                                 << " succeeded";
                W& w = nb::cast<W&>(value, false);
                T& t = unwrap(w);
                if (!access.get_alloc().is_owned(&t)) {
                    Callstack::throw_with_trace("cannot assign pointer to different memory region");
                }
                access.get<OffsetPtr<T>>() = &t;
                return true;
            }
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
            return false;
        };
        py::ToPythonFn to_python_fn = [](const FieldView& field_view, py::ToPythonMode mode) {
            T* t_ptr = field_view.get<OffsetPtr<T>>().get();
            if (t_ptr == nullptr) { return nb::none(); }
            return field_map_to_python(t_ptr->field_map, mode);
        };
        py::ToPythonCastFn to_python_cast_fn = [](const FieldView& field_view) {
            T* t_ptr = field_view.get<OffsetPtr<T>>().get();
            if (t_ptr == nullptr) { return nb::none(); }
            return nb::cast(ref_wrap(*t_ptr), nb::rv_policy::reference);
        };
        register_type<OffsetPtr<T>>(from_python_fn, to_python_fn, to_python_cast_fn);
    }

    template<typename W>
    static void register_struct_type(nb::class_<W>& cls) {
        using T = unwrap_type_t<W>;
        static_assert(std::is_base_of_v<Struct<T>, T>);
        static_assert(std::is_same_v<T, std::remove_cv_t<T>>);
        static_assert(std::is_same_v<typename T::Ref, W>);
        cls.def("__init__",
                [](typename T::Ref* struct_ref) { T::Ref::create_in_place(struct_ref); });
        py::ToPythonFn to_python_fn = [](const FieldView& field_view, py::ToPythonMode mode) {
            Struct<T>& t = field_view.get<T>();
            return field_map_to_python(t.field_map, mode);
        };
        auto from_python_fn = [](FieldAccess<true> access, const nb::handle& value) {
            if (py::copy_cast_from_python<W>(access, value)) { return true; }
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
                T& t = access.get<T>();
                FieldMap<false>& field_map = t.field_map;
                field_map_from_python(field_map, dict, t);
                return true;
            }
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
            return false;
        };
        register_type<W>(from_python_fn, to_python_fn);
        register_struct_ptr_type<W>();
        register_field_map_funcs(cls);
    }

    static const FromPythonFn& get_from_python_fn(type_hash_t type_hash) {
        return get_py_type(type_hash).from_python_fn;
    }

    static const ToPythonFn& get_to_python_fn(type_hash_t type_hash) {
        return get_py_type(type_hash).to_python_fn;
    }

    static const ToPythonCastFn& get_to_python_cast_fn(type_hash_t type_hash) {
        return get_py_type(type_hash).to_python_cast_fn;
    }

    template<typename W>
    static void register_basic_ptr_type() {
        using T = unwrap_type_t<W>;
        static_assert(std::is_same_v<T, W>);
        static_assert(!std::is_pointer_v<T>);
        static_assert(!std::is_class_v<T>);
        auto from_python_fn = [](FieldAccess<true> access, const nb::handle& value) {
            if (access.get_type_hash() == typing::get_type_hash<OffsetPtr<T>>() &&
                nb::isinstance<T>(value)) {
                STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name
                                 << " succeeded";
                T t = nb::cast<T>(value, false);
                *access.get<OffsetPtr<T>>() = t;
                return true;
            }
            STST_LOG_DEBUG() << "converting from type " << typing::get_type<T>().name << " failed";
            return false;
        };
        py::ToPythonFn to_python_fn = [](const FieldView& field_view, py::ToPythonMode) {
            T* t_ptr = field_view.get<OffsetPtr<T>>().get();
            if (t_ptr == nullptr) { return nb::none(); }
            return nb::cast(ref_wrap(*t_ptr));
        };
        py::ToPythonCastFn to_python_cast_fn = [](const FieldView& field_view) {
            T* t_ptr = field_view.get<OffsetPtr<T>>().get();
            if (t_ptr == nullptr) { return nb::none(); }
            return nb::cast(ref_wrap(*t_ptr));
        };
        register_type<OffsetPtr<T>>(from_python_fn, to_python_fn, to_python_cast_fn);
    }

    template<typename W, typename W_py>
    static void register_basic_type() {
        using T = unwrap_type_t<W>;
        static_assert(std::is_same_v<T, W>);
        register_type<W>(default_from_python_fn<W, W_py>, default_to_python_fn<W, W_py>,
                         default_to_python_cast_fn<W>);
        register_basic_ptr_type<W>();
    }

    template<typename W>
    static void register_complex_type_funcs(nb::class_<W>& cls) {
        using T = unwrap_type_t<W>;
        static_assert(!std::is_pointer_v<T>);
        cls.def("to_yaml", [](W& w) { return YAML::Dump(FieldView{unwrap(w)}.to_yaml()); });
        cls.def("__repr__", [](W& w) {
            std::ostringstream str;
            FieldView{unwrap(w)}.to_text(str);
            return str.str();
        });
        cls.def("copy",
                [](W& w) { return to_python(FieldView{unwrap(w)}, ToPythonMode::NON_RECURSIVE); });
        cls.def("deepcopy",
                [](W& w) { return to_python(FieldView{unwrap(w)}, ToPythonMode::RECURSIVE); });
        cls.def("__copy__",
                [](W& w) { return to_python(FieldView{unwrap(w)}, ToPythonMode::NON_RECURSIVE); });
        cls.def("__deepcopy__", [](W& w, nb::handle&) {
            return to_python(FieldView{unwrap(w)}, ToPythonMode::RECURSIVE);
        });
        cls.def("__eq__", [](W& w, nb::handle& other) {
            if (const W* other_ = try_cast<W>(other)) { return unwrap(w) == unwrap(*other_); }
            return false;
        });
        cls.def("read_lock", [](W& w) { return unwrap(w).read_lock(); }, nb::rv_policy::move);
        cls.def("write_lock", [](W& w) { return unwrap(w).write_lock(); }, nb::rv_policy::move);
    }

    template<typename W>
    static W* try_cast(const nb::handle& value) {
        try {
            return &nb::cast<W&>(value, false);
        } catch (const nb::cast_error&) { return nullptr; }
    }

    template<typename W>
    static bool copy_cast_from_python(FieldAccess<true> access, const nb::handle& value) {
        using T = unwrap_type_t<W>;
        // check that a wrapper type (or String) is provided:
        static_assert(std::is_same_v<std::remove_reference_t<wrap_type_w<W>>, W>);

        W* value_cpp_ptr = try_cast<W>(value);
        if (value_cpp_ptr == nullptr) { return false; }
        T& value_cpp = unwrap(*value_cpp_ptr);
        T& field_cpp = access.get<T>();
        STST_LOG_DEBUG() << "at type " << typing::get_type<T>().name;
        if (&value_cpp == &field_cpp) {
            STST_LOG_DEBUG() << "copying to itself";
            return true;
        }
        STST_LOG_DEBUG() << "copying " << typing::get_type<T>().name << " from " << &value_cpp
                         << " to " << &field_cpp;
        field_cpp = value_cpp;
        return true;
    }

    template<typename W>
    static void register_field_map_funcs(nb::class_<W>& cls) {
        using T = unwrap_type_t<W>;
        register_complex_type_funcs<W>(cls);

        cls.def("__getstate__", [](W& w) {
            CallstackEntry entry{"py::__getstate__()"};
            nb::object obj = field_map_to_python(unwrap(w).field_map, py::ToPythonMode::RECURSIVE);
            return obj;
        });

        cls.def("__dir__", [](W& w) {
            nb::list slots;
            const auto& field_map = unwrap(w).field_map;
            for (shr_string_idx str_idx: field_map.get_slots()) {
                slots.append(nb::str(field_map.get_alloc().strings().get(str_idx)->c_str()));
            }
            return slots;
        });

        cls.def("__len__", [](W& w) { return unwrap(w).field_map.get_slots().size(); });

        if constexpr (std::is_base_of_v<FieldType<T>, T> && !std::is_same_v<W, StructStoreShared>) {
            cls.def("__setstate__", [](W& w, nb::handle value) {
                CallstackEntry entry{"py::__setstate__()"};
                using T = unwrap_type_t<W>;
                static_assert(std::is_same_v<typename T::Ref, W>);
                // create temporary field to be able to call from_python()
                Field* field = static_alloc.allocate<Field>();
                new (field) Field;
                auto access = FieldAccess<true>{*field, static_alloc, nullptr};
                // ensure that the field has the desired type T
                access.get<T>();
                from_python(access, value, "<root>");
                new (&w) typename T::Ref{std::move(*field), static_alloc};
                static_alloc.deallocate(field);
            });
        } else {
            cls.def("__setstate__", [](T&, nb::handle) {
                throw std::runtime_error("cannot unpickle type " + std::string(typeid(T).name()));
            });
        }

        cls.def(
                "__getattr__",
                [](W& w, const std::string& name) { return get_field(unwrap(w).field_map, name); },
                nb::arg("name"));

        cls.def(
                "__setattr__",
                [](W& w, const std::string& name, const nb::handle& value) {
                    auto& val = unwrap(w);
                    auto lock = val.write_lock();
                    return set_field(val.field_map, name, value, val);
                },
                nb::arg("name"), nb::arg("value").none());

        cls.def(
                "__getitem__",
                [](W& w, const std::string& name) {
                    // todo: when returning a field, there should be a read lock on the parent StructStore
                    // => attach a read lock to the return value?
                    return get_field(unwrap(w).field_map, name);
                },
                nb::arg("name"));

        cls.def(
                "__setitem__",
                [](W& w, const std::string& name, const nb::handle& value) {
                    auto& val = unwrap(w);
                    auto lock = val.write_lock();
                    return set_field(val.field_map, name, value, val);
                },
                nb::arg("name"), nb::arg("value").none());

        cls.def("empty", [](W& w) { return unwrap(w).field_map.empty(); });
    }

    template<typename W>
    static void register_structstore_funcs(nb::class_<W>& cls) {
        register_field_map_funcs<W>(cls);

        cls.def("clear", [](W& w) { unwrap(w).clear(); });

        cls.def("check", [](W& w) {
            STST_LOG_DEBUG() << "checking from python ...";
            w.check();
        });

        cls.def(
                "__delattr__",
                [](W& w, const std::string& name) {
                    return unwrap(w).field_map.remove(name.c_str());
                },
                nb::arg("name"));

        cls.def(
                "__delitem__",
                [](W& w, const std::string& name) {
                    return unwrap(w).field_map.remove(name.c_str());
                },
                nb::arg("name"));
    }

    static nb::object to_python(const FieldView& field_view, ToPythonMode mode);

    static nb::object to_python_cast(const FieldView& field_view);

    // for unmanaged fields
    static void from_python(FieldAccess<false> access, const nb::handle& value,
                            const std::string& field_name);

    // for managed fields
    static void from_python(FieldAccess<true> access, const nb::handle& value,
                            const std::string& field_name);
};

} // namespace structstore

#endif

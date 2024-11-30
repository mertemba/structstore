#include "structstore/stst_py.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>

using namespace structstore;

namespace nb = nanobind;

std::unordered_map<uint64_t, const py::PyType>& py::get_py_types() {
    static auto* from_python_fns = new std::unordered_map<uint64_t, const py::PyType>();
    return *from_python_fns;
}

const py::PyType& py::get_py_type(uint64_t type_hash) {
    try {
        return py::get_py_types().at(type_hash);
    } catch (const std::out_of_range&) {
        std::ostringstream str;
        str << "could not find Python type information for type '" << typing::get_type(type_hash).name << "'";
        throw std::runtime_error(str.str());
    }
}

__attribute__((__visibility__("default"))) nb::object
py::field_map_to_python(const FieldMapBase& field_map, py::ToPythonMode mode) {
    auto dict = nb::dict();
    for (HashString str: field_map.get_slots()) {
        auto key = nb::cast<std::string>(str.str);
        if (mode == py::ToPythonMode::RECURSIVE) {
            dict[key] = py::to_python(field_map.at(str), py::ToPythonMode::RECURSIVE);
        } else { // non-recursive convert
            dict[key] = py::to_python_cast(field_map.at(str));
        }
    }
    return dict;
}

__attribute__((__visibility__("default"))) nb::object py::to_python(const Field& field,
                                                                    ToPythonMode mode) {
    if (field.empty()) {
        return nb::none();
    }
    py::ToPythonFn to_python_fn = py::get_to_python_fn(field.get_type_hash());
    return to_python_fn(field, mode);
}

__attribute__((__visibility__("default"))) nb::object py::to_python_cast(const Field& field) {
    if (field.empty()) {
        return nb::none();
    }
    py::ToPythonCastFn to_python_cast_fn = py::get_to_python_cast_fn(field.get_type_hash());
    return to_python_cast_fn(field);
}

__attribute__((__visibility__("default"))) void
py::from_python(FieldAccess<false> access, const nb::handle& value, const std::string& field_name) {
    if (value.is_none()) { throw nb::value_error("cannot assign None to unmanaged field"); }
    if (access.get_field().empty()) {
        throw nb::value_error("internal error: unmanaged field is empty");
    }
    STST_LOG_DEBUG() << "at field " << field_name << " of type "
                     << typing::get_type(access.get_type_hash()).name;
    auto from_python_fn = py::get_from_python_fn(access.get_type_hash());
    bool success = from_python_fn(access.to_managed_access(), value);
    if (!success) {
        std::ostringstream msg;
        msg << "cannot assign value of type '" << nb::cast<std::string>(nb::str(value.type()))
            << "' to field '" << field_name << "' of type '"
            << typing::get_type(access.get_type_hash()).name << "'";
        throw nb::type_error(msg.str().c_str());
    }
}

__attribute__((__visibility__("default"))) void
py::from_python(FieldAccess<true> access, const nb::handle& value, const std::string& field_name) {
    if (value.is_none()) {
        access.clear();
        return;
    }
    if (!access.get_field().empty()) {
        STST_LOG_DEBUG() << "at field " << field_name << " of type " << typing::get_type(access.get_type_hash()).name;
#ifndef NDEBUG
        // access.check();
#endif
        auto from_python_fn = py::get_from_python_fn(access.get_type_hash());
        bool success = from_python_fn(access, value);
        if (success) {
            return;
        }
    } else {
        STST_LOG_DEBUG() << "at empty field " << field_name;
        for (const auto& [type_hash, py_type]: py::get_py_types()) {
            bool success = py_type.from_python_fn(access, value);
            if (success) {
                return;
            }
        }
    }
    std::ostringstream msg;
    msg << "cannot assign value of type '" << nb::cast<std::string>(nb::str(value.type()))
        << "' to field '" << field_name << "' of type '" << typing::get_type(access.get_type_hash()).name << "'";
    throw nb::type_error(msg.str().c_str());
}

__attribute__((__visibility__("default"))) nb::object py::get_field(const FieldMapBase& field_map,
                                                                    const std::string& name) {
    const Field* field = field_map.try_get_field(HashString{name.c_str()});
    if (field == nullptr) {
        throw nb::attribute_error();
    }
    return to_python_cast(*field);
}

__attribute__((__visibility__("default"))) void py::set_field(FieldMap<false>& field_map,
                                                              const std::string& name,
                                                              const nb::handle& value,
                                                              const FieldTypeBase& parent_field) {
    STST_LOG_DEBUG() << "setting field to type " << nb::repr(value.type()).c_str();
    Field* field = field_map.try_get_field(HashString{name.c_str()});
    if (field == nullptr) { throw nb::attribute_error(); }
    auto access = FieldAccess<false>{*field, field_map.get_alloc(), &parent_field};
    from_python(access, value, name);
}

__attribute__((__visibility__("default"))) void py::set_field(FieldMap<true>& field_map,
                                                              const std::string& name,
                                                              const nb::handle& value,
                                                              const FieldTypeBase& parent_field) {
    STST_LOG_DEBUG() << "setting field to type " << nb::repr(value.type()).c_str();
    auto access = FieldAccess<true>{field_map[HashString{name.c_str()}], field_map.get_alloc(),
                                    &parent_field};
    from_python(access, value, name);
}

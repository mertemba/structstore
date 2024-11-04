#include "structstore/structstore.hpp"
#include "structstore/stst_py.hpp"

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

nb::object py::structstore_to_python(StructStore& store, py::ToPythonMode mode) {
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
}

nb::object py::to_python(const Field& field, ToPythonMode mode) {
    if (field.empty()) {
        return nb::none();
    }
    py::ToPythonFn to_python_fn = py::get_to_python_fn(field.get_type_hash());
    return to_python_fn(field, mode);
}

nb::object py::to_python_cast(const Field& field) {
    if (field.empty()) {
        return nb::none();
    }
    py::ToPythonCastFn to_python_cast_fn = py::get_to_python_cast_fn(field.get_type_hash());
    return to_python_cast_fn(field);
}

__attribute__((__visibility__("default")))
void py::from_python(FieldAccess access, const nb::handle& value, const std::string& field_name) {
    if (value.is_none()) {
        access.clear();
        return;
    }
    if (!access.get_field().empty()) {
        STST_LOG_DEBUG() << "at field " << field_name << " of type " << typing::get_type(access.get_type_hash()).name;
#ifndef NDEBUG
        access.check();
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

nb::object py::get_field(StructStore& store, const std::string& name) {
    auto lock = store.read_lock();
    Field* field = store.try_get_field(HashString{name.c_str()});
    if (field == nullptr) {
        throw nb::attribute_error();
    }
    return to_python_cast(*field);
}

__attribute__((__visibility__("default")))
void py::set_field(StructStore& store, const std::string& name, const nb::handle& value) {
    auto lock = store.write_lock();
    STST_LOG_DEBUG() << "setting field to type " << nb::repr(value.type()).c_str();
    from_python(store[name.c_str()], value, name);
}

ScopedLock py::lock(StructStore& store) {
    return ScopedLock{store.get_mutex()};
}

void py::clear(StructStore& store) {
    auto lock = store.write_lock();
    store.clear();
}

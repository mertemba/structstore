#include "structstore/structstore.hpp"
#include "structstore/stst_py.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>

using namespace structstore;

namespace nb = nanobind;

nb::object bindings::SimpleNamespace{};

__attribute__((__visibility__("default")))
std::unordered_map<uint64_t, bindings::FromPythonFn>& bindings::get_from_python_fns() {
    static auto* from_python_fns = new std::unordered_map<uint64_t, bindings::FromPythonFn>();
    return *from_python_fns;
}

__attribute__((__visibility__("default")))
std::unordered_map<uint64_t, bindings::ToPythonFn>& bindings::get_to_python_fns() {
    static auto* to_python_fns = new std::unordered_map<uint64_t, bindings::ToPythonFn>();
    return *to_python_fns;
}

nb::object structstore::to_python(const StructStoreField& field, bool recursive) {
    if (field.empty()) {
        return nb::none();
    }
    uint64_t type_hash = field.get_type_hash();
    try {
        bindings::ToPythonFn to_python_fn = bindings::get_to_python_fns().at(type_hash);
        return to_python_fn(field, recursive);
    } catch (const std::out_of_range&) {
        std::ostringstream str;
        str << "error at get_to_python_fns().at() for type '" << typing::get_type_name(type_hash) << "'";
        throw std::runtime_error(str.str());
    }
}

__attribute__((__visibility__("default")))
void structstore::from_python(FieldAccess access, const nb::handle& value, const std::string& field_name) {
    if (value.is_none()) {
        access.clear();
        return;
    }
    if (!access.get_field().empty()) {
        auto from_python_fn = bindings::get_from_python_fns().at(access.get_type_hash());
        bool success = from_python_fn(access, value);
        if (success) {
            return;
        }
    } else {
        for (const auto& [type_hash, from_python_fn]: bindings::get_from_python_fns()) {
            bool success = from_python_fn(access, value);
            if (success) {
                return;
            }
        }
    }
    std::ostringstream msg;
    msg << "cannot assign value of type '" << nb::cast<std::string>(nb::str(value.type()))
        << "' to field '" << field_name << "' of type '" << typing::get_type_name(access.get_type_hash()) << "'";
    throw nb::type_error(msg.str().c_str());
}

nb::object bindings::__slots__(StructStore& store) {
    auto ret = nb::list();
    for (const auto& str: store.get_slots()) {
        ret.append(str.str);
    }
    return ret;
}

nb::object bindings::get_field(StructStore& store, const std::string& name) {
    auto lock = store.read_lock();
    StructStoreField* field = store.try_get_field(HashString{name.c_str()});
    if (field == nullptr) {
        throw nb::attribute_error();
    }
    return to_python(*field, false);
}

void bindings::set_field(StructStore& store, const std::string& name, const nb::object& value) {
    auto lock = store.write_lock();
    from_python(store[name.c_str()], value, name);
}

ScopedLock bindings::lock(StructStore& store) {
    return ScopedLock{store.get_mutex()};
}

void bindings::clear(StructStore& store) {
    auto lock = store.write_lock();
    store.clear();
}

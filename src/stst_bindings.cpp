#include "structstore/structstore.hpp"
#include "structstore/stst_bindings.hpp"

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

nb::object structstore::to_object(const StructStoreField& field, bool recursive) {
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

nb::object structstore::to_object(const List& list, bool recursive) {
    auto pylist = nb::list();
    for (const StructStoreField& field: list) {
        pylist.append(to_object(field, recursive));
    }
    return pylist;
}

nb::object structstore::to_object(const StructStore& store, bool recursive) {
    auto obj = bindings::SimpleNamespace();
    for (const auto& name: store.slots) {
        nb::setattr(obj, name.str, to_object(store.fields.at(name), recursive));
    }
    return obj;
}

__attribute__((__visibility__("default")))
void structstore::from_object(FieldAccess access, const nb::handle& value, const std::string& field_name) {
    if (value.is_none()) {
        access.clear();
        return;
    }
    std::cout << "at field " << field_name << std::endl;
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
    msg << "field '" << field_name << "' has unsupported type '" << nb::cast<std::string>(nb::str(value.type()))
        << "'";
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
    return to_object(*field, false);
}

void bindings::set_field(StructStore& store, const std::string& name, const nb::object& value) {
    auto lock = store.write_lock();
    from_object(store[name.c_str()], value, name);
}

ScopedLock bindings::lock(StructStore& store) {
    store.get_mutex().lock();
    return ScopedLock{store.get_mutex()};
}

nb::object bindings::__repr__(StructStore& store) {
    auto lock = store.read_lock();
    std::ostringstream str;
    str << store;
    return nb::cast(str.str());
}

nb::object bindings::copy(StructStore& store) {
    auto lock = store.read_lock();
    return to_object(store, false);
}

nb::object bindings::deepcopy(StructStore& store) {
    auto lock = store.read_lock();
    return to_object(store, true);
}

void bindings::clear(StructStore& store) {
    auto lock = store.write_lock();
    store.clear();
}

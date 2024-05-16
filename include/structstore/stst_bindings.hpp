#ifndef STST_BINDINGS_HPP
#define STST_BINDINGS_HPP

#include "structstore/stst_hashstring.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>

#include <nanobind/nanobind.h>

namespace structstore::bindings {

using FromPythonFn = std::function<bool(FieldAccess, const nanobind::handle&)>;
using ToPythonFn = std::function<nanobind::object(const StructStoreField&, bool recursive)>;

std::vector<FromPythonFn>& get_from_python_fns();

std::unordered_map<uint64_t, ToPythonFn>& get_to_python_fns();

bool register_from_python_fn(const FromPythonFn& from_python_fn) {
    get_from_python_fns().push_back(from_python_fn);
    return true;
}

template<typename T>
bool register_to_python_fn(const ToPythonFn& to_python_fn) {
    bool success = get_to_python_fns().insert(
            {typing::get_type_hash<T>(), to_python_fn}).second;
    if (!success) {
        throw std::runtime_error("type already registered");
    }
    return true;
}

}

#endif
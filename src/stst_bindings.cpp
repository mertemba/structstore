#include "structstore/structstore.hpp"
#include "structstore/stst_bindings.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/string.h>

using namespace structstore;

namespace nb = nanobind;

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

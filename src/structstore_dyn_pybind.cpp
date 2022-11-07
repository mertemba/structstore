#include <pybind11/pybind11.h>
#include "structstore_dyn.hpp"
#include "structstore_python.hpp"
#include "structstore_shared.hpp"

namespace py = pybind11;

PYBIND11_MODULE(structstore, m) {
    auto cls = register_pystruct<StructStoreDyn>(m, "StructStore", true);
    cls.def("add_int", &StructStoreDyn::add_field<int>);
    cls.def("add_str", &StructStoreDyn::add_field<std::string>);
    cls.def("add_bool", &StructStoreDyn::add_field<bool>);
//    cls.def("add_store", &StructStoreDyn::add_field<StructStoreBase>);

    auto shcls = py::class_<StructStoreShared<StructStoreDyn>>(m, "StructStoreShared");
    shcls.def(pybind11::init<const std::string&>());
    shcls.def("get_store", &StructStoreShared<StructStoreDyn>::operator*, pybind11::return_value_policy::reference);
}

#include <pybind11/pybind11.h>
#include "structstore_dyn.hpp"
#include "structstore_pybind.hpp"

namespace py = pybind11;
using namespace structstore;

PYBIND11_MODULE(structstore, m) {
    auto basecls = register_pystruct<StructStoreBase>(m, "StructStoreBase", nullptr);

    auto cls = register_pystruct<StructStoreDyn<>>(m, "StructStore", &basecls);
    cls.def("add_int", &StructStoreDyn<>::add_field<int>);
    cls.def("add_str", &StructStoreDyn<>::add_field<std::string>);
    cls.def("add_bool", &StructStoreDyn<>::add_field<bool>);
    cls.def("add_store", &StructStoreDyn<>::add_field<StructStoreDyn<0>>,
            pybind11::return_value_policy::reference_internal);

    auto subcls = register_pystruct<StructStoreDyn<0>>(m, "SubStructStore", &basecls);
    subcls.def("add_int", &StructStoreDyn<0>::add_field<int>);
    subcls.def("add_str", &StructStoreDyn<0>::add_field<std::string>);
    subcls.def("add_bool", &StructStoreDyn<0>::add_field<bool>);
    subcls.def("add_store", &StructStoreDyn<0>::add_field<StructStoreDyn<0>>,
               pybind11::return_value_policy::reference_internal);
}

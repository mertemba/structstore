#include <iostream>
#include <structstore_dyn.hpp>
#include <structstore_shared.hpp>

namespace stst = structstore;

int main() {
    stst::StructStoreDyn state;
    int& num = state.add_field<int>("num");
    num = 5;

    auto& substate = state.add_field<stst::StructStoreDyn<0>>("substate");
    substate.add_field<int>("subnum") = 77;
    std::cout << "complete state: " << state << std::endl;

    stst::StructStoreShared<stst::StructStoreDyn<>> shdata("/dyn_shdata_store");
    std::cout << "shared data: " << *shdata << std::endl;
    shdata->add_field<int>("num") = 53;
    shdata[H("num")] = 53;
    std::cout << "shared data: " << *shdata << std::endl;
    std::cout << "allocated bytes: " << shdata->allocated_size() << std::endl;
    return 0;
}

#include <iostream>
#include "structstore_dyn.hpp"
#include "structstore_shared.hpp"

int main() {
    StructStoreDyn state;
    int& num = state.add_field<int>("num");
    num = 5;
    std::cout << "State: " << state << std::endl;
    std::cout << YAML::Dump(to_yaml(state)) << std::endl;
    state[H("num")] = 42;
    std::cout << "num is " << (int) state["num"] << std::endl;

    auto& substate = state.add_field<StructStoreDyn<0>>("substate");
    substate.add_field<int>("subnum") = 77;
    std::cout << "subnum: " << substate[H("subnum")] << std::endl;
    std::cout << "complete state: " << state << std::endl;

    StructStoreShared<StructStoreDyn<>> shdata("/dyn_shdata_store");
    std::cout << "shared data: " << *shdata << std::endl;
    std::cout << "got data" << std::endl;
    shdata->add_field<int>("num");
    shdata[H("num")] = 53;
    std::cout << "shared data: " << *shdata << std::endl;
    std::cout << "shared data alloc: " << shdata->allocated_size() << std::endl;
    return 0;
}

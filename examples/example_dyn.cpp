#include <iostream>
#include <structstore_dyn.hpp>
#include <structstore_shared.hpp>

namespace stst = structstore;

struct DynSubsettings : stst::StructStoreDyn<0> {
    int& subnum = add_field<int>("subnum") = 42;
    stst::arena_str& substr = add_field<stst::arena_str>("substr") = "bar";

    explicit DynSubsettings(stst::Arena& arena) : stst::StructStoreDyn<0>(arena) {}
};

struct DynSettings : stst::StructStoreDyn<> {
    int& num = add_field<int>("num") = 5;
    bool& flag = add_field<bool>("flag") = true;
    stst::arena_str& str = add_field<stst::arena_str>("str") = "foo";
    DynSubsettings& subsettings = add_field<DynSubsettings>("subsettings");
};

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

    stst::StructStoreShared<DynSettings> dyn_settings("/dyn_settings");
    std::cout << "semi-dynamic struct: " << *dyn_settings << std::endl;

    return 0;
}

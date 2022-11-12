#include <iostream>
#include <structstore.hpp>
#include <structstore_shared.hpp>

namespace stst = structstore;

struct Subsettings : public stst::StructStore {
    stst::StructStore& store;

    int& subnum = store.get<int>("subnum") = 42;
    stst::string& substr = store.get<stst::string>("substr") = "bar";

    explicit Subsettings(stst::StructStore& store) : store(store) {}
};

struct Settings {
    stst::StructStore& store;

    int& num = store.get<int>("num") = 5;
    bool& flag = store.get<bool>("flag") = true;
    stst::string& str = store.get<stst::string>("str") = "foo";
    Subsettings subsettings{store.get<stst::StructStore>("subsettings")};

    explicit Settings(stst::StructStore& store) : store(store) {}
};

int main() {
    stst::StructStore state;
    int& num = state.get<int>("num");
    num = 5;

    auto& substate = state.get<stst::StructStore>("substate");
    substate.get<int>("subnum") = 77;
    std::cout << "complete state: " << state << std::endl;

    stst::StructStoreShared shdata_store("/shdata_store");
    std::cout << "shared data: " << *shdata_store << std::endl;
    shdata_store->get<int>("num") = 52;
    shdata_store[H("num")] = 53;
    std::cout << "allocated bytes: " << shdata_store->allocated_size() << std::endl;

    stst::StructStoreShared shsettings_store("/shsettings_store");
    Settings shsettings{*shsettings_store};
    std::cout << "semi-dynamic struct: " << *shsettings_store << std::endl;

    return 0;
}

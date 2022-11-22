#include <iostream>
#include <structstore.hpp>
#include <structstore_shared.hpp>

namespace stst = structstore;

struct Subsettings {
    stst::StructStore& store;

    int& subnum = store["subnum"] = 42;
    stst::string& substr = store["substr"] = (const char*) "bar";

    explicit Subsettings(stst::StructStore& store) : store(store) {}
};

struct Settings {
    stst::StructStore& store;

    int& num = store["num"] = 5;
    double& value = store["value"] = 3.14;
    bool& flag = store["flag"] = true;
    stst::string& str = store["str"] = (const char*) "foo";
    Subsettings subsettings{store.get<stst::StructStore>("subsettings")};

    explicit Settings(stst::StructStore& store) : store(store) {}
};

int main() {
    stst::StructStore store;
    int& num = store["num"];
    num = 5;
    std::cout << "store: " << store << std::endl;

    Settings settings{store};
    settings.num = 42;
    settings.subsettings.subnum = 43;
    std::cout << "subsettings: " << store["subsettings"] << std::endl;
    std::cout << "settings: " << store << std::endl;

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

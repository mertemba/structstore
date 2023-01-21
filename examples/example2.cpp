#include <iostream>
#include <structstore.hpp>
#include <structstore_shared.hpp>
#include <structstore_containers.hpp>
#include <thread>

namespace stst = structstore;

struct Subsettings {
    int subnum = 42;
    std::string substr = "bar";
};

struct Settings {
    int num = 5;
    double value = 3.14;
    bool flag = true;
    std::string str = "foo";
    Subsettings subsettings;
};

void to_store(stst::StructStore& store, const Subsettings& subsettings) {
    store["subnum"] = subsettings.subnum;
    store["substr"] = subsettings.substr;
}

void to_store(stst::StructStore& store, const Settings& settings) {
    store["num"] = settings.num;
    store["value"] = settings.value;
    store["flag"] = settings.flag;
    store["str"] = settings.str;
    to_store(store["subsettings"], settings.subsettings);
}

void from_store(stst::StructStore& store, Subsettings& subsettings) {
    subsettings.subnum = store["subnum"];
    subsettings.substr = store["substr"].get_str();
}

void from_store(stst::StructStore& store, Settings& settings) {
    settings.num = store["num"];
    settings.value = store["value"];
    settings.flag = store["flag"];
    settings.str = store["str"].get_str();
    from_store(store["subsettings"], settings.subsettings);
}

int main() {
    Settings settings;
    settings.num = 42;
    settings.subsettings.subnum = 43;

    stst::StructStore store;
    to_store(store, settings);
    std::cout << store << std::endl;

    stst::StructStoreShared shsettings_store("/shsettings_store");
    to_store(*shsettings_store, settings);

    Settings settings2;
    from_store(*shsettings_store, settings2);
    return 0;
}

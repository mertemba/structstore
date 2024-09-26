#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

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

TEST(StructStoreTestBasic, structInStaticStore) {
    Settings settings;

    stst::StructStore store(stst::static_alloc);
    to_store(store, settings);
    std::stringstream str;
    str << store;
    EXPECT_EQ(str.str(), "{\"num\":5,\"value\":3.14,\"flag\":1,\"str\":foo,\"subsettings\":{\"subnum\":42,\"substr\":bar,},}");
}

TEST(StructStoreTestBasic, structInSharedStore) {
    Settings settings;
    settings.num = 42;
    settings.subsettings.subnum = 43;

    stst::StructStoreShared shsettings_store("/shsettings_store");
    to_store(*shsettings_store, settings);

    Settings settings2;
    from_store(*shsettings_store, settings2);
    EXPECT_EQ(settings.num, settings2.num);
    EXPECT_EQ(settings.subsettings.subnum, settings2.subsettings.subnum);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

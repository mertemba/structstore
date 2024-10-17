#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

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
    Subsettings subsettings{store.substore("subsettings")};

    explicit Settings(stst::StructStore& store) : store(store) {}
};

TEST(StructStoreTestBasic, refStructInLocalStore) {
    stst::StructStore store(stst::static_alloc);
    int& num = store["num"];
    num = 5;
    std::stringstream str;
    str << store;
    EXPECT_EQ(str.str(), "{\"num\":5,}");

    Settings settings{store};
    settings.num = 42;
    settings.subsettings.subnum = 43;

    stst::List& list = store["list"];
    list.push_back(5);
    list.push_back(42);
    for (int& i: list) {
        ++i;
    }
    EXPECT_EQ(list.size(), 2);

    stst::List& strlist = store["strlist"];
    strlist.push_back((const char*) "foo");
    for (stst::string& str: strlist) {
        str += "bar";
    }
    EXPECT_EQ(strlist.size(), 1);
    EXPECT_EQ(strlist[0].get<stst::string>().size(), 6);

    std::string yaml_str = (std::stringstream() << to_yaml(store)).str();
    EXPECT_EQ(yaml_str, "num: 42\nvalue: 3.1400000000000001\nflag: true\nstr: foo\nsubsettings:\n  subnum: 43\n  substr: bar\nlist:\n  - 6\n  - 43\nstrlist:\n  - foobar");
}

TEST(StructStoreTestBasic, refStructInSharedStore) {
    stst::StructStoreShared shdata_store("/shdata_store");
    std::stringstream str;
    str << *shdata_store;
    EXPECT_EQ(str.str(), "{}");

    shdata_store->get<int>("num") = 52;
    EXPECT_EQ(shdata_store["num"].get<int>(), 52);
}

TEST(StructStoreTestBasic, sharedStore) {
    stst::StructStoreShared shsettings_store("/shsettings_store");
    Settings shsettings{*shsettings_store};
    std::stringstream str;
    str << *shsettings_store;
    EXPECT_EQ(str.str(), "{\"num\":5,\"value\":3.14,\"flag\":1,\"str\":foo,\"subsettings\":{\"subnum\":42,\"substr\":bar,},}");
}

TEST(StructStoreTestBasic, cmpEqual) {
    stst::StructStore store1(stst::static_alloc);
    Settings settings1{store1};
    stst::StructStore store2(stst::static_alloc);
    Settings settings2{store2};
    EXPECT_EQ(store1, store2);
    settings2.subsettings.substr += '.';
    EXPECT_NE(store1, store2);
}

TEST(StructStoreTestBasic, cmpEqualShared) {
    stst::StructStoreShared store1("/shsettings_store1");
    Settings settings1{*store1};
    stst::StructStoreShared store2("/shsettings_store2");
    Settings settings2{*store2};
    EXPECT_EQ(*store1, *store2);
    EXPECT_EQ(store1, store2);
    settings2.subsettings.substr += '.';
    EXPECT_NE(*store1, *store2);
    EXPECT_NE(store1, store2);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

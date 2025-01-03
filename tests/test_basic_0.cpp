#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

namespace stst = structstore;

struct Subsettings {
    stst::StructStore& store;

    int& subnum = store["subnum"] = 42;
    stst::String& substr = store["substr"] = "bar";

    explicit Subsettings(stst::StructStore& store) : store(store) {}
};

struct Settings {
    stst::StructStore& store;

    int& num = store["num"] = 5;
    double& value = store["value"] = 3.14;
    bool& flag = store["flag"] = true;
    stst::String& str = store["str"] = "foo";
    Subsettings subsettings{store.substore("subsettings")};

    explicit Settings(stst::StructStore& store) : store(store) {}
};

TEST(StructStoreTestBasic, refStructInLocalStore) {
    stst::StructStore store(stst::static_alloc);
    store["num"] = 5;
    std::ostringstream str;
    str << store;
    EXPECT_EQ(str.str(), "{\"num\":5,}");
    store.remove("num");
    EXPECT_TRUE(store.empty());

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
    list.push_back(stst::StructStore(stst::static_alloc));
    list.check();
    EXPECT_EQ(list.size(), 3);
    for (stst::Field& field: list) {
        field.to_text(std::cout);
        std::cout << std::endl;
    }

    stst::List& strlist = store["strlist"];
    strlist.push_back("foo");
    for (stst::String& str: strlist) {
        str += "bar";
    }
    EXPECT_EQ(strlist.size(), 1);
    EXPECT_EQ(strlist[0].get<stst::String>().size(), 6);

    std::string yaml_str = (std::ostringstream() << store.to_yaml()).str();
    EXPECT_EQ(yaml_str, "num: 42\nvalue: 3.1400000000000001\nflag: true\nstr: foo\nsubsettings:\n  subnum: 43\n  substr: bar\nlist:\n  - 6\n  - 43\n  - {}\nstrlist:\n  - foobar");

    store.check();
}

TEST(StructStoreTestBasic, refStructInSharedStore) {
    stst::StructStoreShared shdata_store("/shdata_store");
    std::ostringstream str;
    str << *shdata_store;
    EXPECT_EQ(str.str(), "{}");

    shdata_store->get<int>("num") = 52;
    EXPECT_EQ(shdata_store["num"].get<int>(), 52);

    shdata_store.check();
}

TEST(StructStoreTestBasic, sharedStore) {
    stst::StructStoreShared shsettings_store("/shsettings_store");
    Settings shsettings{*shsettings_store};
    std::ostringstream str;
    str << *shsettings_store;
    EXPECT_EQ(str.str(), "{\"num\":5,\"value\":3.14,\"flag\":1,\"str\":foo,\"subsettings\":{\"subnum\":42,\"substr\":bar,},}");
    shsettings_store.check();
}

TEST(StructStoreTestBasic, cmpEqual) {
    stst::StructStore store1(stst::static_alloc);
    Settings settings1{store1};
    stst::StructStore store2(stst::static_alloc);
    Settings settings2{store2};
    EXPECT_EQ(store1, store2);
    settings2.subsettings.substr += '.';
    EXPECT_NE(store1, store2);
    store1.check();
    store2.check();
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
    store1.check();
    store2.check();
}

TEST(StructStoreTestBasic, copyStore) {
    stst::StructStore store(stst::static_alloc);
    int& num = store["num"];
    num = 5;
    store["str"].get_str() = "foo";
    stst::StructStore store2 = store;
    EXPECT_EQ(store2["num"].get<int>(), 5);
    store.check();
    store2.check();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include "mystruct0.hpp"

#include <gtest/gtest.h>
#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestStruct, structCreation) {
    auto store_ptr = stst::create<stst::StructStore>();
    stst::StructStore& store = *store_ptr;
    store.get<Frame>("frame");
    store.check();

    std::string yaml_str = (std::ostringstream() << store.to_yaml()).str();
    EXPECT_EQ(yaml_str, "frame:\n  t: 0\n  flag: false\n  t_ptr: <double_ptr>");

    auto store_ptr2 = stst::create<stst::StructStore>();
    stst::StructStore& store2 = *store_ptr2;
    store2 = store;
    EXPECT_EQ(store, store2);
    store2["frame"].get<Frame>().t = 1.0;
    EXPECT_NE(store, store2);
    store2.check();
}

TEST(StructStoreTestStruct, structBasicFuncs) {
    auto f_ptr = stst::create<Frame>();
    Frame& f = *f_ptr;
    f.flag = true;
    f.t = 2.5;
    auto f2_ptr = stst::create<Frame>();
    Frame& f2 = *f2_ptr;
    f2 = f;
    EXPECT_EQ(f, f2);

    std::string str = (std::ostringstream() << f).str();
    EXPECT_EQ(str, "{\"t\":2.5,\"flag\":1,\"t_ptr\":<double_ptr>,}");
}

TEST(StructStoreTestStruct, sharedStruct) {
    stst::StructStoreShared shdata_store("/shdata_store");
    shdata_store->get<Frame>("frame");
    shdata_store.check();

    std::string yaml_str = (std::ostringstream() << shdata_store->to_yaml()).str();
    EXPECT_EQ(yaml_str, "frame:\n  t: 0\n  flag: false\n  t_ptr: <double_ptr>");

    auto store_ptr2 = stst::create<stst::StructStore>();
    stst::StructStore& store2 = *store_ptr2;
    store2 = *shdata_store;
    EXPECT_EQ(*shdata_store, store2);
    store2["frame"].get<Frame>().t = 1.0;
    EXPECT_NE(*shdata_store, store2);
    store2.check();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

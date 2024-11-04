#include "structstore/stst_structstore.hpp"
#include <gtest/gtest.h>

#include <structstore/structstore.hpp>
#include "mystruct0.hpp"

namespace stst = structstore;

TEST(StructStoreTestStruct, structCreation) {
    stst::StructStore store(stst::static_alloc);
    Frame f;
    store["frame"] = f;

    std::string yaml_str = (std::stringstream() << store.to_yaml()).str();
    EXPECT_EQ(yaml_str, "frame:\n  t: 0\n  flag: false");

    stst::StructStore store2 = store;
    EXPECT_EQ(store, store2);
    store2["frame"].get<Frame>().t = 1.0;
    EXPECT_NE(store, store2);
}

TEST(StructStoreTestStruct, sharedStruct) {
    stst::StructStoreShared shdata_store("/shdata_store");
    Frame f;
    shdata_store["frame"] = f;

    std::string yaml_str = (std::stringstream() << shdata_store->to_yaml()).str();
    EXPECT_EQ(yaml_str, "frame:\n  t: 0\n  flag: false");

    stst::StructStore store2 = *shdata_store;
    EXPECT_EQ(*shdata_store, store2);
    store2["frame"].get<Frame>().t = 1.0;
    EXPECT_NE(*shdata_store, store2);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

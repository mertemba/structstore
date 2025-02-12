#include "mystruct1.hpp"

#include <gtest/gtest.h>
#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestStruct, nestedStruct) {
    auto store_ref = stst::StructStore::create();
    stst::StructStore& store = *store_ref;
    store.get<Track>("track");
    store.check();

    std::string yaml_str = (std::ostringstream() << store.to_yaml()).str();
    EXPECT_EQ(yaml_str,
              "track:\n  frame1:\n    t: 0\n    flag: false\n    t_ptr: <double_ptr>\n  frame2:\n    "
              "t: 0\n    flag: false\n    t_ptr: <double_ptr>\n  frame_ptr: <Frame_ptr>");

    auto store_ref2 = stst::StructStore::create();
    stst::StructStore& store2 = *store_ref2;
    store2 = store;
    EXPECT_EQ(store, store2);
    store2["track"].get<Track>().frame1.t = 1.0;
    EXPECT_NE(store, store2);
    store2.check();
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

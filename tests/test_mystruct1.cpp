#include "mystruct1.hpp"

#include <gtest/gtest.h>
#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestStruct, nestedStruct) {
    auto store_ptr = stst::create<stst::StructStore>();
    stst::StructStore& store = *store_ptr;
    store.get<Track>("track");
    store.check();

    std::string yaml_str = (std::ostringstream() << store.to_yaml()).str();
    EXPECT_EQ(yaml_str,
              "track:\n  frame1:\n    t: 0\n    flag: false\n    t_ptr: <double_ptr>\n  frame2:\n    "
              "t: 0\n    flag: false\n    t_ptr: <double_ptr>\n  frame_ptr: <Frame_ptr>");

    auto store_ptr2 = stst::create<stst::StructStore>();
    stst::StructStore& store2 = *store_ptr2;
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

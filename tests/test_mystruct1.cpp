#include "structstore/stst_structstore.hpp"
#include <gtest/gtest.h>

#include "mystruct0.hpp"
#include "mystruct1.hpp"
#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestStruct, nestedStruct) {
    stst::StructStore store(stst::static_alloc);
    Track track;
    store["track"] = track;

    std::string yaml_str = (std::ostringstream() << store.to_yaml()).str();
    EXPECT_EQ(yaml_str,
              "track:\n  frame1:\n    t: 0\n    flag: false\n  frame2:\n    t: 0\n    flag: false");

    stst::StructStore store2 = store;
    EXPECT_EQ(store, store2);
    store2["track"].get<Track>().frame1.t = 1.0;
    EXPECT_NE(store, store2);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

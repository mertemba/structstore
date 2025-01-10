#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestAlloc, structSizes) {
    EXPECT_EQ(sizeof(stst::FieldTypeBase), 16);
    EXPECT_EQ(sizeof(stst::Field), 16);
    EXPECT_EQ(sizeof(stst::String), 56);
    EXPECT_EQ(sizeof(stst::StructStore), 128);
    EXPECT_EQ(sizeof(stst::SharedAlloc), 32);
}

TEST(StructStoreTestAlloc, bigAlloc) {
    stst::CallstackEntry entry{"prefix"};
    EXPECT_THROW(
            try { stst::static_alloc.allocate(10'000'000); } catch (const std::runtime_error& e) {
                EXPECT_STREQ(e.what(),
                             "prefix: insufficient space in sh_alloc region, requested: 10000000");
                throw;
            },
            std::runtime_error);
}

TEST(StructStoreTestAlloc, isOwned) {
    EXPECT_FALSE(stst::static_alloc.is_owned(nullptr));
    EXPECT_FALSE(stst::static_alloc.is_owned(&stst::static_alloc));
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestAlloc, structSizes) {
    EXPECT_EQ(sizeof(stst::FieldTypeBase), 16);
    EXPECT_EQ(sizeof(stst::Field), 16);
    EXPECT_EQ(sizeof(stst::String), 56);
    EXPECT_EQ(sizeof(stst::StructStore), 136);
    EXPECT_EQ(sizeof(stst::SharedAlloc), 64);
}

TEST(StructStoreTestAlloc, bigAlloc) {
    stst::CallstackEntry entry{"prefix"};
#ifndef NDEBUG
#define EXPECTED_STR "prefix: insufficient space in sh_alloc region, requested: 10000000"
#else
#define EXPECTED_STR "insufficient space in sh_alloc region, requested: 10000000"
#endif
    EXPECT_THROW(
            try { stst::static_alloc.allocate(10'000'000); } catch (const std::runtime_error& e) {
                EXPECT_STREQ(e.what(), EXPECTED_STR);
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

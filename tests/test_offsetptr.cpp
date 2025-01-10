#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestOffsetPtr, basic) {
    int foo = 42;
    stst::OffsetPtr<int> foo_ptr = &foo;
    EXPECT_TRUE(foo_ptr);
    EXPECT_EQ(foo_ptr.get(), &foo);
    EXPECT_EQ((foo_ptr = nullptr).get(), nullptr);
    EXPECT_EQ((foo_ptr = &foo).get(), &foo);
    EXPECT_EQ(*foo_ptr, foo);
    EXPECT_EQ((foo_ptr + 1).get(), &foo + 1);
    EXPECT_EQ((foo_ptr - 1).get(), &foo - 1);
    EXPECT_EQ((++foo_ptr).get(), &foo + 1);
    EXPECT_EQ(foo_ptr.get(), &foo + 1);
    EXPECT_EQ((--foo_ptr).get(), &foo);
    EXPECT_EQ(foo_ptr.get(), &foo);
    EXPECT_EQ((foo_ptr + 1) - foo_ptr, 1);
    EXPECT_EQ(foo_ptr - (foo_ptr - 1), 1);

    stst::OffsetPtr<int> nil_ptr;
    EXPECT_FALSE(nil_ptr);
    nil_ptr = nullptr;
    EXPECT_FALSE(nil_ptr);
}

TEST(StructStoreTestOffsetPtr, string) {
    std::string str = "foo";
    using offset_string = std::basic_string<char, std::char_traits<char>, stst::StlAllocator<char>>;
    offset_string str_{stst::StlAllocator<char>{stst::static_alloc}};
    str_ = str;
}

TEST(StructStoreTestOffsetPtr, containers) {
    std::vector<int, stst::StlAllocator<int>> v{stst::StlAllocator<char>{stst::static_alloc}};
    v.reserve(10);
    v.push_back(10);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

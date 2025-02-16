#include <gtest/gtest.h>

#include <structstore/structstore.hpp>

namespace stst = structstore;

TEST(StructStoreTestUtils, callstack) {
    stst::CallstackEntry entry0{"first"};
    stst::CallstackEntry entry1{"second"};
    { stst::CallstackEntry entry2{"third"}; }
    std::string what;
    try {
        stst::Callstack::warn_with_trace("inner warning");
        stst::Callstack::throw_with_trace("inner error");
    } catch (const std::runtime_error& e) { what = e.what(); }
#ifndef NDEBUG
    EXPECT_EQ(what, "first: second: inner error");
#else
    EXPECT_EQ(what, "inner error");
#endif
}

TEST(StructStoreTestUtils, logging) {
    STST_LOG_WARN() << "test warning";
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

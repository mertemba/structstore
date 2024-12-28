#include "structstore/stst_alloc.hpp"

using namespace structstore;

static constexpr size_t static_alloc_size = 1 << 20;
MiniMalloc structstore::static_alloc(std::malloc(static_alloc_size), static_alloc_size);

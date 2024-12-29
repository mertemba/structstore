#include "structstore/stst_alloc.hpp"

using namespace structstore;

static constexpr size_t static_alloc_size = 1 << 20;
MiniMalloc structstore::static_alloc(std::malloc(static_alloc_size), static_alloc_size);

MiniMalloc::MiniMalloc(void* buffer, size_t size)
    : mm{nullptr}, buffer{(byte*) buffer}, blocksize{size}, string_storage{nullptr} {
    if (buffer == nullptr) { return; }
    mm = init_mini_malloc(buffer, size);
    string_storage = (StringStorage*) allocate(sizeof(StringStorage));
    new (string_storage) StringStorage(*this);
}

MiniMalloc::~MiniMalloc() noexcept(false) {
    string_storage->~StringStorage();
    deallocate(string_storage);
    mm_assert_all_freed(mm);
}

const shr_string* StringStorage::internalize(const std::string& str) {
    return &*data.insert(shr_string{str, StlAllocator<int>{mm_alloc}}).first;
}

const shr_string* StringStorage::get(const std::string& str) {
    auto it = data.find(shr_string{str, StlAllocator<int>{mm_alloc}});
    if (it != data.end()) { return &*it; }
    return nullptr;
}

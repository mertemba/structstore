#include "structstore/stst_alloc.hpp"

using namespace structstore;

struct ManagedMalloc {
    void* data;
    ManagedMalloc(size_t size) : data{std::malloc(size)} {}
    ~ManagedMalloc() { std::free(data); }
};

static constexpr size_t static_alloc_size = 1 << 20;
ManagedMalloc static_alloc_data{static_alloc_size};
SharedAlloc structstore::static_alloc{static_alloc_data.data, static_alloc_size};

SharedAlloc::SharedAlloc(void* buffer, size_t size)
    : mm{(mini_malloc*) buffer}, blocksize{size}, string_storage{nullptr} {
    if (buffer == nullptr) { return; }
    init_mini_malloc(mm.get(), size);
    string_storage = allocate<StringStorage>();
    new (string_storage.get()) StringStorage(*this);
}

SharedAlloc::~SharedAlloc() noexcept(false) {
    string_storage->~StringStorage();
    deallocate(string_storage.get());
    mm_assert_all_freed(mm.get());
}

const shr_string* StringStorage::internalize(const std::string& str) {
    shr_string str_{str, StlAllocator<>{sh_alloc}};
    {
        ScopedLock<false> lock{mutex};
        auto it = data.find(str_);
        if (it != data.end()) { return &*it; }
    }
    ScopedLock<true> lock{mutex};
    const shr_string& found_str = *data.insert(str_).first;
    return &found_str;
}

const shr_string* StringStorage::get(const std::string& str) {
    ScopedLock<false> lock{mutex};
    shr_string str_{str, StlAllocator<>{sh_alloc}};
    auto it = data.find(str_);
    if (it != data.end()) { return &*it; }
    return nullptr;
}

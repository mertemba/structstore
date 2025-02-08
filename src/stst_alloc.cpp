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

StringStorage::StringStorage(SharedAlloc& sh_alloc)
    : map{StlAllocator<int>(sh_alloc)}, data{StlAllocator<int>(sh_alloc)} {
    // element 0 is none
    data.emplace_back();
}

shr_string_idx StringStorage::internalize(const std::string& str, SharedAlloc& sh_alloc) {
    if (shr_string_idx found_idx = get_idx(str, sh_alloc)) { return found_idx; }
    shr_string str_{str, StlAllocator{sh_alloc}};
    ScopedLock<true> lock{mutex};
    auto [it, inserted] = map.emplace(str_, 0);
    if (inserted) {
        it->second = data.size();
        data.emplace_back(str_);
    }
    return it->second;
}

shr_string_idx StringStorage::get_idx(const std::string& str, SharedAlloc& sh_alloc) const {
    shr_string str_{str, StlAllocator{sh_alloc}};
    ScopedLock<false> lock{mutex};
    auto it = map.find(str_);
    if (it != map.end()) { return it->second; }
    return 0;
}

const shr_string* StringStorage::get(shr_string_idx idx) const { return &data[idx]; }

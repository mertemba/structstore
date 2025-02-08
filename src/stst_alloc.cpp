#include "structstore/stst_alloc.hpp"
#include "structstore/stst_utils.hpp"
#include <limits>

using namespace structstore;

struct ManagedSharedAlloc {
    SharedAlloc* sh_alloc;
    ManagedSharedAlloc(size_t size) {
        size_t sh_alloc_size = sizeof(SharedAlloc);
        if (sh_alloc_size % 8) { sh_alloc_size += 8 - sh_alloc_size % 8; }
        uint8_t* data = (uint8_t*) std::malloc(size);
        sh_alloc = new (data) SharedAlloc(data + sh_alloc_size, size - sh_alloc_size);
    }
    ~ManagedSharedAlloc() { std::free(sh_alloc); }
};

static constexpr size_t static_alloc_size = 1 << 20;
static ManagedSharedAlloc managed_static_alloc{static_alloc_size};
SharedAlloc& structstore::static_alloc = *managed_static_alloc.sh_alloc;

SharedAlloc::SharedAlloc(void* buffer, size_t size)
    : mm{(mini_malloc*) buffer}, blocksize{(uint32_t) size}, string_storage{nullptr} {
    if (buffer == nullptr) { return; }
    stst_assert(size < (1ull << 31));
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

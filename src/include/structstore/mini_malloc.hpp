#ifndef MINI_MALLOC_HPP
#define MINI_MALLOC_HPP

#include <cstddef>

namespace structstore {

struct mini_malloc;

// this function must be called exactly once before the first call to sh_alloc or mm_free,
// with a block of memory and its size as parameters
void init_mini_malloc(mini_malloc* sh_alloc, size_t blocksize);

// returns a pointer to size bytes of memory, aligned to 8 bytes
void* mm_allocate(mini_malloc* sh_alloc, size_t size);

// free a block of memory previously allocated by sh_alloc
void mm_free(mini_malloc* sh_alloc, void* ptr);

void mm_assert_all_freed(mini_malloc* sh_alloc);

} // namespace structstore

#endif

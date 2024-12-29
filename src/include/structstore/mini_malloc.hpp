#ifndef MINI_MALLOC_HPP
#define MINI_MALLOC_HPP

#include <cstddef>

namespace structstore {

struct mini_malloc;

// this function must be called exactly once before the first call to mm_alloc or mm_free,
// with a block of memory and its size as parameters
mini_malloc* init_mini_malloc(void* buffer, size_t blocksize);

// returns a pointer to size bytes of memory, aligned to 8 bytes
void* mm_allocate(mini_malloc*, size_t size);

// free a block of memory previously allocated by mm_alloc
void mm_free(mini_malloc*, void* ptr);

void mm_assert_all_freed(mini_malloc*);

} // namespace structstore

#endif

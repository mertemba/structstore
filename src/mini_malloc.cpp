#include "structstore/mini_malloc.hpp"
#include "structstore/stst_utils.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>

#define ALIGN 8
#define SIZES_COUNT 59
#define MAX_PTRDIFF_VAL ((INT64_C(1) << 31) - 1)

// for an allocated node, only the first 8 bytes of the struct are needed.
#define ALLOC_NODE_SIZE 8
#define ALLOCATED_FLAG UINT32_C(1)

namespace structstore {

typedef uint8_t byte;
typedef uint32_t size_type;
typedef int16_t size_index_type;
typedef int32_t ptrdiff_type;

static size_type sizes[SIZES_COUNT];

typedef struct mini_malloc {
    ptrdiff_type _head;
    ptrdiff_type free_nodes[SIZES_COUNT];
} mini_malloc;

typedef struct memnode {
    // allocatable size in bytes
    size_type size;
    // 0 if this is the first node in block; MSB set if node is allocated (ALLOCATED_FLAG)
    size_type prev_node_size;
    ptrdiff_type d_next_free_node;
    ptrdiff_type d_prev_free_node;
} memnode;

} // namespace structstore

using namespace structstore;

static inline bool is_allocated(memnode* node) {
    return (node->prev_node_size & ALLOCATED_FLAG) != 0;
}

static inline void set_allocated(memnode* node) { node->prev_node_size |= ALLOCATED_FLAG; }

static inline void set_unallocated(memnode* node) { node->prev_node_size &= ~ALLOCATED_FLAG; }

static memnode* get_next_free_node(memnode* node) {
    assert(!is_allocated(node));
    if (node->d_next_free_node == 0) { return NULL; }
    return (memnode*) ((byte*) node + node->d_next_free_node);
}

static memnode* get_prev_free_node(memnode* node) {
    assert(!is_allocated(node));
    if (node->d_prev_free_node == 0) { return NULL; }
    return (memnode*) ((byte*) node + node->d_prev_free_node);
}

static void set_next_free_node(memnode* node, memnode* next_free_node) {
    assert(!is_allocated(node));
    if (next_free_node == NULL) {
        node->d_next_free_node = 0;
        return;
    }
    ptrdiff_t ptrdiff = (byte*) next_free_node - (byte*) node;
    assert(ptrdiff > -MAX_PTRDIFF_VAL);
    assert(ptrdiff < MAX_PTRDIFF_VAL);
    node->d_next_free_node = (ptrdiff_type) ptrdiff;
}

static void set_prev_free_node(memnode* node, memnode* prev_free_node) {
    assert(!is_allocated(node));
    if (prev_free_node == NULL) {
        node->d_prev_free_node = 0;
        return;
    }
    ptrdiff_t ptrdiff = (byte*) prev_free_node - (byte*) node;
    assert(ptrdiff > -MAX_PTRDIFF_VAL);
    assert(ptrdiff < MAX_PTRDIFF_VAL);
    node->d_prev_free_node = (ptrdiff_type) ptrdiff;
}

static size_type get_prev_node_size(memnode* node) {
    return node->prev_node_size & ~ALLOCATED_FLAG;
}

static void set_prev_node_size(memnode* node, size_type prev_node_size) {
    assert(is_allocated(node));
    node->prev_node_size = prev_node_size | ALLOCATED_FLAG;
}

#undef ALLOCATED_FLAG

static int uint64_log2(uint64_t n) {
#define S(k)                                                                                       \
    if (n >= (UINT64_C(1) << k)) {                                                                 \
        i += k;                                                                                    \
        n >>= k;                                                                                   \
    }
    int i = -(n == 0);
    S(32);
    S(16);
    S(8);
    S(4);
    S(2);
    S(1);
    return i;
#undef S
}

static size_index_type get_size_index_upper(size_type size_) {
    assert(size_ % ALIGN == 0);
    uint64_t size = size_ / ALIGN;
    if (size <= 4) { return size - 1; }
    if (size > (1ull << 16)) { return SIZES_COUNT - 1; }
    size *= size;
    size *= size;
    return uint64_log2(size - 1ull) - 5;
}

static size_index_type get_size_index_lower(size_type size) {
    size_index_type idx = get_size_index_upper(size);
    assert(idx >= 0 && idx < SIZES_COUNT);
    while (size < sizes[idx]) {
        --idx;
        assert(idx >= 0 && idx < SIZES_COUNT);
    }
    assert(idx >= 0 && idx < SIZES_COUNT);
    return idx;
}

static memnode* get_free_nodes_head(mini_malloc* mm, size_index_type size_index) {
    return (memnode*) ((byte*) &(mm->free_nodes[size_index]) - ALLOC_NODE_SIZE);
}

static memnode* get_free_nodes_first(mini_malloc* mm, size_index_type size_index) {
    return get_next_free_node(get_free_nodes_head(mm, size_index));
}

static void attach_free_nodes(memnode* node1, memnode* node2) {
    if (node1 != NULL) { set_next_free_node(node1, node2); }
    if (node2 != NULL) { set_prev_free_node(node2, node1); }
}

static memnode* get_prev_node(memnode* node) {
    size_type prev_node_size = get_prev_node_size(node);
    if (prev_node_size == 0) { return NULL; }
    return (memnode*) (((byte*) node) - (prev_node_size + ALLOC_NODE_SIZE));
}

static memnode* get_next_node(memnode* node) {
    memnode* next_node = (memnode*) (((byte*) node) + (node->size + ALLOC_NODE_SIZE));
    // last node in a block has size 0
    if (next_node->size == 0) { return NULL; }
    return next_node;
}

static void prepend_free_node(mini_malloc* mm, memnode* node, size_index_type size_index) {
    memnode* old_first_free = get_free_nodes_first(mm, size_index);
    attach_free_nodes(get_free_nodes_head(mm, size_index), node);
    attach_free_nodes(node, old_first_free);
}

mini_malloc* structstore::init_mini_malloc(void* buffer, size_t blocksize) {
    mini_malloc* mm = (mini_malloc*) buffer;
    static_assert((sizeof(mini_malloc) % ALIGN) == 0);
    buffer = (byte*) buffer + sizeof(mini_malloc);
    blocksize -= sizeof(mini_malloc);

    // ensure alignment
    static_assert((ALLOC_NODE_SIZE % ALIGN) == 0);
    static_assert(sizeof(memnode) == 16);

    // init size array
    for (uint32_t bits = 1; bits <= 64; ++bits) {
        uint64_t size = ((uint64_t) (std::pow(2.0, bits / 4.0) + 0.001)) * ALIGN;
        size_index_type idx = get_size_index_upper(size);
        assert(idx >= 0 && idx < SIZES_COUNT);
        sizes[idx] = size;
    }

    size_type block_node_size = blocksize - 2 * ALLOC_NODE_SIZE;
    // fill free_nodes array
    for (size_index_type size_index = -1; size_index < SIZES_COUNT; size_index++) {
        mm->free_nodes[size_index] = 0;
    }
    // allocate first block
    memnode* block_node = (memnode*) buffer;
    assert(block_node != NULL);
    block_node->d_next_free_node = 0;
    block_node->prev_node_size = 0; // also sets to unallocated
    set_prev_free_node(block_node, get_free_nodes_head(mm, SIZES_COUNT - 1));
    block_node->size = block_node_size;
    memnode* last_node = (memnode*) (((byte*) block_node) + (block_node->size + ALLOC_NODE_SIZE));
    set_allocated(last_node);
    last_node->size = 0;
    set_next_free_node(get_free_nodes_head(mm, SIZES_COUNT - 1), block_node);
    assert((byte*) last_node + ALLOC_NODE_SIZE == (byte*) buffer + blocksize);
    return mm;
}

void* structstore::mm_allocate(mini_malloc* mm, size_t size) {
    if (size == 0) return NULL;

    if (size % ALIGN) { size += ALIGN - size % ALIGN; }
    size_index_type size_index = get_size_index_upper(size);
    assert(size_index >= 0 && size_index < SIZES_COUNT);
    if (size_index < SIZES_COUNT - 1) { size = sizes[size_index]; }
    memnode* node = NULL;
    // search for first free node
    do {
        assert(size_index >= 0 && size_index < SIZES_COUNT);
        node = get_free_nodes_first(mm, size_index);
    } while (node == NULL && ++size_index < SIZES_COUNT);

    if (node == NULL) { return NULL; }
    assert(node->size > 0);
    if (size_index == SIZES_COUNT - 1) {
        // linearly search all big nodes for a big enough one
        while (node != NULL && node->size < size) { node = get_next_free_node(node); }
        if (node == NULL) { return NULL; }
    }
    if (node->size < size) {
        // not enough memory left
        return NULL;
    }

    // split node if big enough
    int32_t left_size = node->size - size - ALLOC_NODE_SIZE;
    assert(left_size >= -ALLOC_NODE_SIZE);
    if (left_size >= ALIGN) {
        size_index_type left_size_index = get_size_index_lower(left_size);
        memnode* new_node = (memnode*) (((byte*) node) + size + ALLOC_NODE_SIZE);
        new_node->size = left_size;
        node->size = size;
        new_node->d_prev_free_node = 0;
        new_node->d_next_free_node = 0;
        new_node->prev_node_size = node->size; // also sets to unallocated
        memnode* next_node = get_next_node(new_node);
        if (next_node != NULL) { set_prev_node_size(next_node, new_node->size); }
        // prepend new_node to free nodes list:
        prepend_free_node(mm, new_node, left_size_index);
        assert(get_free_nodes_first(mm, left_size_index) == new_node);
    }
    // remove node from free nodes list:
    attach_free_nodes(get_prev_free_node(node), get_next_free_node(node));
    set_allocated(node);
    return ((byte*) node) + ALLOC_NODE_SIZE;
}


static void join_with_next(mini_malloc* mm, memnode* node) {
    if (node == NULL || is_allocated(node)) { return; }
    memnode* next_node = get_next_node(node);
    if (next_node == NULL || is_allocated(next_node)) { return; }
    node->size += next_node->size + ALLOC_NODE_SIZE;
    // remove node from free nodes list:
    attach_free_nodes(get_prev_free_node(node), get_next_free_node(node));
    // remove next_node from nodes lists:
    attach_free_nodes(get_prev_free_node(next_node), get_next_free_node(next_node));

    next_node = get_next_node(node);
    if (next_node != NULL) { set_prev_node_size(next_node, node->size); }
    // prepend node to free nodes list:
    size_index_type size_index = get_size_index_lower(node->size);
    prepend_free_node(mm, node, size_index);
}

void structstore::mm_free(mini_malloc* mm, void* ptr) {
    if (!ptr) return;

    memnode* node = (memnode*) (((byte*) ptr) - ALLOC_NODE_SIZE);
    size_index_type size_index = get_size_index_lower(node->size);
    set_unallocated(node);
    // prepend node to free nodes list:
    prepend_free_node(mm, node, size_index);

    join_with_next(mm, node);
    join_with_next(mm, get_prev_node(node));
}

void structstore::mm_assert_all_freed(mini_malloc* mm) {
    memnode* block_node = (memnode*) ((byte*) mm + sizeof(mini_malloc));
    memnode* node = block_node;
    bool found_allocated = false;
    while (node != nullptr) {
        if (is_allocated(node)) {
            found_allocated = true;
            STST_LOG_ERROR() << "found leaked memory block " << node << " of size " << node->size;
        }
        node = get_next_node(node);
    }
    if (found_allocated) { throw std::runtime_error("found leaked memory blocks"); }
}

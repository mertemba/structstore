#ifndef STST_ALLOC_HPP
#define STST_ALLOC_HPP

#include "structstore/stst_lock.hpp"
#include "structstore/stst_utils.hpp"

#include <cassert>
#include <cmath>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace structstore {

class MiniMalloc {
    static constexpr int ALIGN = 8;
    static constexpr int SIZES_COUNT = 59;
    static constexpr int64_t MAX_PTRDIFF_VAL = ((INT64_C(1) << 31) - 1);

    // for an allocated node, only the first 8 bytes of the struct are needed.
    static constexpr int ALLOC_NODE_SIZE = 8;
    static constexpr uint32_t ALLOCATED_FLAG = UINT32_C(1);

    // private type definitions
    typedef uint8_t byte;
    typedef uint32_t size_type;
    typedef int16_t size_index_type;
    typedef int32_t ptrdiff_type;

    struct memnode {
        size_type size; // allocatable size in bytes
        size_type prev_node_size; // 0 if this is the first node in block; MSB set if node is allocated (ALLOCATED_FLAG)
        ptrdiff_type d_next_free_node;
        ptrdiff_type d_prev_free_node;
    };

    // member variables
    SpinMutex mutex;
    byte* const buffer;
    ptrdiff_type (*free_nodes)[SIZES_COUNT] = nullptr;
    size_type sizes[SIZES_COUNT];
    const size_t blocksize;
    size_t allocated;
    memnode* block_node;

    // member methods
    static inline bool is_allocated(memnode* node) {
        return (node->prev_node_size & ALLOCATED_FLAG) != 0;
    }

    static inline void set_allocated(memnode* node) {
        node->prev_node_size |= ALLOCATED_FLAG;
    }

    static inline void set_unallocated(memnode* node) {
        node->prev_node_size &= ~ALLOCATED_FLAG;
    }

    static memnode* get_next_free_node(memnode* node) {
        assert(!is_allocated(node));
        if (node->d_next_free_node == 0) {
            return nullptr;
        }
        return (memnode*) (node->d_next_free_node + (byte*) node);
    }

    static memnode* get_prev_free_node(memnode* node) {
        assert(!is_allocated(node));
        if (node->d_prev_free_node == 0) {
            return nullptr;
        }
        return (memnode*) (node->d_prev_free_node + (byte*) node);
    }

    static void set_next_free_node(memnode* node, memnode* next_free_node) {
        assert(!is_allocated(node));
        if (next_free_node == nullptr) {
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
        if (prev_free_node == nullptr) {
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

    static int uint64_log2(uint64_t n) {
#define S(k)                       \
    if (n >= (UINT64_C(1) << k)) { \
        i += k;                    \
        n >>= k;                   \
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
        if (size <= 4) {
            return size - 1;
        }
        if (size > (1ull << 16)) {
            return SIZES_COUNT - 1;
        }
        size *= size;
        size *= size;
        return uint64_log2(size - 1ull) - 5;
    }

    size_index_type get_size_index_lower(size_type size) {
        size_index_type idx = get_size_index_upper(size);
        assert(idx >= 0 && idx < SIZES_COUNT);
        while (size < sizes[idx]) {
            --idx;
            assert(idx >= 0 && idx < SIZES_COUNT);
        }
        assert(idx >= 0 && idx < SIZES_COUNT);
        return idx;
    }

    memnode* get_free_nodes_head(size_index_type size_index) {
        return (memnode*) ((size_type*) &((*free_nodes)[size_index]) - 2);
    }

    memnode* get_free_nodes_first(size_index_type size_index) {
        return get_next_free_node(get_free_nodes_head(size_index));
    }

    static void attach_free_nodes(memnode* node1, memnode* node2) {
        if (node1 != nullptr) {
            set_next_free_node(node1, node2);
        }
        if (node2 != nullptr) {
            set_prev_free_node(node2, node1);
        }
    }

    static memnode* get_prev_node(memnode* node) {
        size_type prev_node_size = get_prev_node_size(node);
        if (prev_node_size == 0) {
            return nullptr;
        }
        return (memnode*) (((byte*) node) - (prev_node_size + ALLOC_NODE_SIZE));
    }

    static memnode* get_next_node(memnode* node) {
        auto* next_node = (memnode*) (((byte*) node) + (node->size + ALLOC_NODE_SIZE));
        // last node in a block has size 0
        if (next_node->size == 0) {
            return nullptr;
        }
        return next_node;
    }

    void prepend_free_node(memnode* node, size_index_type size_index) {
        memnode* old_first_free = get_free_nodes_first(size_index);
        attach_free_nodes(get_free_nodes_head(size_index), node);
        attach_free_nodes(node, old_first_free);
    }

    void init_mini_malloc() {
        byte* ptr = buffer;
        // ensure 8-byte alignment
        static_assert((ALLOC_NODE_SIZE % 8) == 0);
        static_assert(sizeof(memnode) == 16);

        // init block_node_size array
        for (uint32_t bits = 1; bits <= 64; ++bits) {
            uint64_t size = ((uint64_t) (std::pow(2.0, bits / 4.0) + 0.001)) * ALIGN;
            size_index_type idx = get_size_index_upper(size);
            assert(idx >= 0 && idx < SIZES_COUNT);
            sizes[idx] = size;
        }

        free_nodes = nullptr;
        size_type block_node_size = blocksize - 2 * ALLOC_NODE_SIZE;
        // fill free_nodes array
        size_t free_nodes_size = ((sizeof(*free_nodes) + sizeof(ptrdiff_type) + ALIGN - 1) / ALIGN) * ALIGN;
        block_node_size -= free_nodes_size;
        free_nodes = (ptrdiff_type (*)[SIZES_COUNT]) ((ptrdiff_type*) ptr + 1);
        ptr += free_nodes_size;
        for (size_index_type size_index = -1; size_index < SIZES_COUNT; size_index++) {
            (*free_nodes)[size_index] = 0;
        }
        assert(ptr >= (byte*) &(*free_nodes)[SIZES_COUNT]);
        // allocate first block
        block_node = (memnode*) ptr;
        assert(block_node != nullptr);
        block_node->d_next_free_node = 0;
        block_node->prev_node_size = 0; // also sets to unallocated
        set_prev_free_node(block_node, get_free_nodes_head(SIZES_COUNT - 1));
        block_node->size = block_node_size;
        auto* last_node = (memnode*) (((byte*) block_node) + (block_node->size + ALLOC_NODE_SIZE));
        set_allocated(last_node);
        last_node->size = 0;
        set_next_free_node(get_free_nodes_head(SIZES_COUNT - 1), block_node);
        set_zero(block_node);
        assert((byte*) last_node + ALLOC_NODE_SIZE == buffer + blocksize);
    }

    void* mm_alloc(size_t size) {
        if (size == 0) return nullptr;

        if (size % ALIGN) {
            size += ALIGN - size % ALIGN;
        }
        size_index_type size_index = get_size_index_upper(size);
        if (size_index < SIZES_COUNT - 1) {
            assert(size_index >= 0 && size_index < SIZES_COUNT);
            size = sizes[size_index];
        }
        memnode* node;
        // search for first free node
        do {
            node = get_free_nodes_first(size_index);
        } while (node == nullptr && ++size_index < SIZES_COUNT);

        if (node == nullptr) {
            return nullptr;
        }
        assert(node->size > 0);
        if (size_index == SIZES_COUNT - 1) {
            // linearly search all big nodes for a big enough one
            while (node != nullptr && node->size < size) {
                node = get_next_free_node(node);
            }
            if (node == nullptr) {
                return nullptr;
            }
        }
        if (node->size < size) {
            // not enough memory left
            return nullptr;
        }

        // split node if big enough
        int32_t left_size = node->size - size - ALLOC_NODE_SIZE;
        assert(left_size >= -ALLOC_NODE_SIZE);
        if (left_size >= ALIGN) {
            size_index_type left_size_index = get_size_index_lower(left_size);
            auto* new_node = (memnode*) (((byte*) node) + size + ALLOC_NODE_SIZE);
            new_node->size = left_size;
            node->size = size;
            new_node->d_prev_free_node = 0;
            new_node->d_next_free_node = 0;
            new_node->prev_node_size = node->size; // also sets to unallocated
            memnode* next_node = get_next_node(new_node);
            if (next_node != nullptr) {
                set_prev_node_size(next_node, new_node->size);
            }
            // prepend new_node to free nodes list:
            prepend_free_node(new_node, left_size_index);
            assert(get_free_nodes_first(left_size_index) == new_node);
        }
        // remove node from free nodes list:
        attach_free_nodes(get_prev_free_node(node), get_next_free_node(node));
        set_allocated(node);
        return ((byte*) node) + ALLOC_NODE_SIZE;
    }

    void join_with_next(memnode* node) {
        if (node == nullptr || is_allocated(node)) {
            return;
        }
        memnode* next_node = get_next_node(node);
        if (next_node == nullptr || is_allocated(next_node)) {
            return;
        }
        node->size += next_node->size + ALLOC_NODE_SIZE;
        // remove node from free nodes list:
        attach_free_nodes(get_prev_free_node(node), get_next_free_node(node));
        // remove next_node from nodes lists:
        attach_free_nodes(get_prev_free_node(next_node), get_next_free_node(next_node));
        // fill header of next_node with zeros to enable better compression:
        std::memset(((byte*) next_node), 0, sizeof(memnode));

        next_node = get_next_node(node);
        if (next_node != nullptr) {
            set_prev_node_size(next_node, node->size);
        }
        // prepend node to free nodes list:
        size_index_type size_index = get_size_index_lower(node->size);
        prepend_free_node(node, size_index);
    }

    void mm_free(void* ptr) {
        if (!ptr) return;

        auto* node = (memnode*) (((byte*) ptr) - ALLOC_NODE_SIZE);
        size_index_type size_index = get_size_index_lower(node->size);
        set_unallocated(node);
        // prepend node to free nodes list:
        prepend_free_node(node, size_index);
        set_zero(node);

        join_with_next(node);
        join_with_next(get_prev_node(node));
    }

    void set_zero(memnode* node) {
        // fill unused memory regions with zeros to enable better compression
        std::memset(((byte*) node) + sizeof(memnode), 0, node->size - (sizeof(memnode) - ALLOC_NODE_SIZE));
    }

public:
    MiniMalloc(size_t size, void* buffer)
        : buffer{(byte*) buffer}, blocksize{size}, allocated{0}, block_node{nullptr} {
        if (buffer == nullptr) {
            return;
        }
        init_mini_malloc();
    }

    ~MiniMalloc() noexcept(false) {
        memnode* node = block_node;
        bool found_allocated = false;
        while (node != nullptr) {
            if (is_allocated(node)) {
                found_allocated = true;
                STST_LOG_ERROR() << "found leaked memory block " << node << " of size "
                                 << node->size;
            }
            node = get_next_node(node);
        }
        if (found_allocated) { throw std::runtime_error("found leaked memory blocks"); }
    }

    MiniMalloc() = delete;

    MiniMalloc(MiniMalloc&&) = delete;

    MiniMalloc(const MiniMalloc&) = delete;

    MiniMalloc& operator=(MiniMalloc&&) = delete;

    MiniMalloc& operator=(const MiniMalloc&) = delete;

    bool init() { return true; }

    void dispose() {}

    void* allocate(size_t field_size) {
        if (field_size == 0) {
            field_size = ALIGN;
        }
        ScopedLock<true> lock{mutex};
        if (free_nodes == nullptr) {
            throw std::runtime_error("internal allocator error: free_nodes not initialized");
        }
        void* ptr = mm_alloc(field_size);
        if (ptr == nullptr) {
            std::ostringstream str;
            str << "insufficient space in mm_alloc region, currently allocated: " << allocated;
            str << ", requested: " << field_size;
            throw std::runtime_error(str.str());
        }
        auto* node = (memnode*) (((byte*) ptr) - ALLOC_NODE_SIZE);
        allocated += node->size;
        return ptr;
    }

    void deallocate(void* ptr) {
        ScopedLock<true> lock{mutex};
        auto* node = (memnode*) (((byte*) ptr) - ALLOC_NODE_SIZE);
        allocated -= node->size;
        mm_free(ptr);
    }

    size_t get_size() const {
        return blocksize;
    }

    size_t get_allocated() const {
        return allocated;
    }

    void assert_owned(const void* ptr) const {
        if (ptr == nullptr) {
            throw std::runtime_error("pointer is NULL");
        }
        if (ptr < buffer || ptr >= buffer + blocksize) {
            throw std::runtime_error("pointer not inside managed memory");
        }
    }
};

extern MiniMalloc static_alloc;

template<typename T = char>
class StlAllocator {
    template<typename U>
    friend class StlAllocator;

    MiniMalloc& mm_alloc;

public:
    using value_type = T;

    explicit StlAllocator(MiniMalloc& a) : mm_alloc(a) {}

    template<typename U>
    StlAllocator(const StlAllocator<U>& other) : mm_alloc(other.mm_alloc) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(mm_alloc.allocate(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t) {
        mm_alloc.deallocate(p);
    }

    // defined in stst_typing.hpp
    inline void construct(T* p);

    void construct(T* p, T&& other) {
        construct(p);
        *p = std::move(other);
    }

    void construct(T* p, const T& other) {
        construct(p);
        *p = other;
    }

    template<typename U>
    bool operator==(StlAllocator<U> const& rhs) const {
        return &mm_alloc == &rhs.mm_alloc;
    }

    template<typename U>
    bool operator!=(StlAllocator<U> const& rhs) const {
        return &mm_alloc != &rhs.mm_alloc;
    }

    MiniMalloc& get_alloc() { return mm_alloc; }
};
} // namespace structstore

#endif

#ifndef STRUCTSTORE_ALLOC_HPP
#define STRUCTSTORE_ALLOC_HPP

#include <type_traits>
#include <scoped_allocator>
#include <unordered_map>

namespace structstore {

struct Arena {
    char* buffer;
    size_t size;
    size_t allocated;
    Arena* extra_arena = nullptr;

    Arena(size_t size, void* buffer) : buffer((char*) buffer), size(size), allocated(0) {}

    bool init() { return true; }

    void dispose() {}

    void* allocate(size_t field_size) {
        field_size = ((field_size + 7) / 8) * 8;
        if (allocated + field_size > size) {
            if (extra_arena != nullptr) {
                std::cout << "allocating from extra arena ..." << std::endl;
                return extra_arena->allocate(field_size);
            }
            throw std::runtime_error("insufficient space in arena region");
        }
        void* ret = buffer + allocated;
        allocated += field_size;
        return ret;
    }

    void deallocate(void* addr, size_t field_size) {}
};

template<typename T>
class ArenaAllocator {
    template<typename U> friend
    struct ArenaAllocator;

    Arena* arena;

public:
    using value_type = T;

    explicit ArenaAllocator(Arena& a) : arena(&a) {}

    template<typename U>
    ArenaAllocator(const ArenaAllocator<U>& other) : arena(other.arena) {}

    T* allocate(std::size_t n) {
        return static_cast<T*>(arena->allocate(n * sizeof(T)));
    }

    void deallocate(T* p, std::size_t n) {
        arena->deallocate(p, n * sizeof(T));
    }

    template<typename U>
    bool operator==(ArenaAllocator<U> const& rhs) const {
        return arena == rhs.arena;
    }

    template<typename U>
    bool operator!=(ArenaAllocator<U> const& rhs) const {
        return arena != rhs.arena;
    }
};

using string = std::basic_string<char, std::char_traits<char>, ArenaAllocator<char>>;

template<class T>
using vector = std::vector<T, ArenaAllocator<T>>;

template<class K, class T>
using unordered_map = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, ArenaAllocator<std::pair<const K, T>>>;

}

#endif

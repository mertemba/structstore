#ifndef STST_HASHSTRING_HPP
#define STST_HASHSTRING_HPP

#include <cstdint>
#include <cstring>
#include <functional>

namespace structstore {

constexpr uint64_t const_hash(const char* input) {
    // FNV1a hash of reversed string
    return *input != 0 ? (const_hash(input + 1) ^ uint64_t((uint8_t) *input)) * 0x100000001b3ull
                       : 0xcbf29ce484222325ull;
}

#ifndef NDEBUG
static_assert(structstore::const_hash("foobar") == 0x4bedc277acafaa72ull);
#endif

struct HashString {
    const char* str;
    uint64_t hash;

    HashString() : str(nullptr), hash(0) {}

    HashString(HashString&& other) : HashString() {
        *this = std::move(other);
    }

    HashString(const HashString& other) : HashString() {
        *this = other;
    }

    HashString& operator=(HashString&& other) {
        std::swap(str, other.str);
        std::swap(hash, other.hash);
        return *this;
    }

    HashString& operator=(const HashString& other) {
        str = other.str;
        hash = other.hash;
        return *this;
    }

    explicit HashString(const char* str) : str(str), hash(const_hash(str)) {}

    HashString(const char* str, uint64_t hash) : str(str), hash(hash) {}

    bool operator==(const HashString& other) const {
        return hash == other.hash && (str == other.str || std::strcmp(str, other.str) == 0);
    }

    bool operator!=(const HashString& other) const {
        return !(*this == other);
    }
};

}

namespace std {
template<>
struct hash<structstore::HashString> {
    std::size_t operator()(const structstore::HashString& s) const noexcept {
        return s.hash;
    }
};
} // namespace std

// macro guarantees compile-time hashing:
#define H(str) (structstore::HashString{str, std::integral_constant<std::size_t, structstore::const_hash(str)>::value})

#endif

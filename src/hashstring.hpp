#ifndef HASHSTRING_HPP
#define HASHSTRING_HPP

#include <cstdint>
#include <cstring>
#include <functional>
#include <ostream>
#include <unordered_set>

constexpr std::size_t const_hash(const char* input) {
    return *input != 0 ? static_cast<std::size_t>(*input) + 33 * const_hash(input + 1) : 5381;
}

struct HashString {
    const char* str;
    std::size_t hash;

    explicit HashString(const char* str) : str(str), hash(const_hash(str)) {}

    HashString(const char* str, std::size_t hash) : str(str), hash(hash) {}

    bool operator==(const HashString& other) const {
        return hash == other.hash && (str == other.str || std::strcmp(str, other.str) == 0);
    }

    bool operator!=(const HashString& other) const {
        return !(*this == other);
    }
};

std::ostream& operator<<(std::ostream& os, const HashString& str);

namespace std {
template<>
struct hash<HashString> {
    std::size_t operator()(const HashString& s) const noexcept {
        return s.hash;
    }
};
} // namespace std

// macro version, guarantees compile-time hashing:
#define H(str) (HashString{str, std::integral_constant<std::size_t, const_hash(str)>::value})

// function version:
//constexpr HashString H(const char* str) {
//    return {str, const_hash(str)};
//}

#endif

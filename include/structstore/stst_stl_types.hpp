#ifndef STST_STL_TYPES_HPP
#define STST_STL_TYPES_HPP

#include "structstore/stst_alloc.hpp"

namespace structstore {

using string = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;

template<class T>
using vector = std::vector<T, StlAllocator<T>>;

template<class K, class T>
using unordered_map = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, StlAllocator<std::pair<const K, T>>>;

}

#endif
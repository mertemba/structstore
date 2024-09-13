// this needs to be included by stst_typing.hpp
#define STST_STL_TYPES_HPP

namespace structstore {

using string = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;

template<class T>
using vector = std::vector<T, StlAllocator<T>>;

template<class K, class T>
using unordered_map = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, StlAllocator<std::pair<const K, T>>>;

template<>
inline YAML::Node to_yaml(const string& str) {
    return YAML::Node(str.c_str());
}

}

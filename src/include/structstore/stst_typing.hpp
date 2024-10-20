#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_alloc.hpp"
#include "structstore/stst_utils.hpp"

#include <functional>
#include <iostream>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace structstore {

template<typename T>
std::ostream& to_text(std::ostream& os, const T& t) {
    return os << t;
}

template<typename T>
YAML::Node to_yaml(const T& t) {
    return YAML::Node(t);
}

template<typename T>
void check(MiniMalloc&, const T&) {}

class Struct;

class typing {
public:
    template<typename T=void>
    using ConstructorFn = std::function<void(MiniMalloc&, T*)>;

    template<typename T=void>
    using DestructorFn = std::function<void(MiniMalloc&, T*)>;

    template<typename T=void>
    using SerializeTextFn = std::function<void(std::ostream&, const T*)>;

    template<typename T=void>
    using SerializeYamlFn = std::function<YAML::Node(const T*)>;

    template <typename T = void>
    using CheckFn = std::function<void(MiniMalloc &, const T *)>;

    template<typename T = void>
    using CmpEqualFn = std::function<bool(const T*, const T*)>;

private:
    template<typename T = void>
    struct FieldType {
        const std::string name;
        ConstructorFn<T> constructor_fn;
        DestructorFn<T> destructor_fn;
        SerializeTextFn<T> serialize_text_fn;
        SerializeYamlFn<T> serialize_yaml_fn;
        CheckFn<T> check_fn;
        CmpEqualFn<T> cmp_equal_fn;

        FieldType(const std::string name) : name{name} {
            if constexpr (!std::is_void_v<T>) {
                if constexpr (std::is_constructible_v<T, MiniMalloc&>) {
                    constructor_fn = mm_alloc_constructor_fn<T>;
                } else if constexpr (std::is_constructible_v<T, const StlAllocator<T>&>) {
                    constructor_fn = stl_alloc_constructor_fn<T>;
                } else {
                    constructor_fn = default_constructor_fn<T>;
                }
                destructor_fn = default_destructor_fn<T>;
                serialize_text_fn = default_serialize_text_fn<T>;
                serialize_yaml_fn = default_serialize_yaml_fn<T>;
                check_fn = default_check_fn<T>;
                cmp_equal_fn = default_cmp_equal_fn<T>;
            } else {
                constructor_fn = [](MiniMalloc&, void*) {};
                destructor_fn = [](MiniMalloc&, void*) {};
                serialize_text_fn = [](std::ostream& os, const void*) -> std::ostream& {
                    return os << "<empty>";
                };
                serialize_yaml_fn = [](const void*) { return YAML::Node(YAML::Null); };
                check_fn = [](MiniMalloc&, const void*) {};
                cmp_equal_fn = [](const void*, const void*) { return true; };
            }
        }

        void assert_complete() {
            if (!constructor_fn) {
                throw std::runtime_error("FieldType has no constructor_fn");
            }
            if (!destructor_fn) {
                throw std::runtime_error("FieldType has no destructor_fn");
            }
            if (!serialize_text_fn) {
                throw std::runtime_error("FieldType has no serialize_text_fn");
            }
            if (!serialize_yaml_fn) {
                throw std::runtime_error("FieldType has no serialize_yaml_fn");
            }
            if (!check_fn) {
                throw std::runtime_error("FieldType has no check_fn");
            }
            if (!cmp_equal_fn) {
                throw std::runtime_error("FieldType has no cmp_equal_fn");
            }
        }

    protected:
        friend class typing;
        const FieldType<void>& cast() const {
            return (FieldType<void>&) *this;
        }
    };

    static std::unordered_map<std::type_index, uint64_t>& get_type_hashes();

    static std::unordered_map<uint64_t, const FieldType<void>>& get_field_types();

    static const FieldType<void>& get_field_type(uint64_t type_hash);

    template<typename T>
    static void default_constructor_fn(MiniMalloc&, T* t) {
        new (t) T();
    }

    template<typename T>
    static void mm_alloc_constructor_fn(MiniMalloc& mm_alloc, T* t) {
        new (t) T(mm_alloc);
    }

    template<typename T>
    static void stl_alloc_constructor_fn(MiniMalloc& mm_alloc, T* t) {
        new (t) T(StlAllocator<T>{mm_alloc});
    }

    template<typename T>
    static void default_destructor_fn(MiniMalloc&, T* t) {
        t->~T();
    }

    template<typename T>
    static void default_serialize_text_fn(std::ostream& os, const T* t) {
        to_text(os, *t);
    }

    template<typename T>
    static void dummy_serialize_text_fn(std::ostream& os, const T*) {
        return os << "{" << typing::get_type_name<T>() << "}";
    }

    template<typename T>
    static YAML::Node default_serialize_yaml_fn(const T* t) {
        return to_yaml(*t);
    }

    template<typename T>
    static void default_check_fn(MiniMalloc& mm_alloc, const T* t) {
        try_with_info("in default check: ", mm_alloc.assert_owned(t););
        check(mm_alloc, *t);
    }

    template<typename T>
    static bool default_cmp_equal_fn(const T* t, const T* other) {
        return *t == *other;
    };

public:
    static void register_common_types();

    static std::runtime_error already_registered_type_error(uint64_t type_hash) {
        std::ostringstream str;
        str << "type already registered: " << get_type_name(type_hash);
        return std::runtime_error(str.str());
    }

    template<typename T>
    static void register_type(const std::string& name) {
        uint64_t type_hash = const_hash(name.c_str());
        if constexpr (std::is_void_v<T>) {
            type_hash = 0;
        } else {
            if (type_hash == 0) {
                std::ostringstream str;
                str << "hash collision between '" << name << "' and empty type";
                throw std::runtime_error(str.str());
            }
        }
        STST_LOG_DEBUG() << "registering type '" << name << "' with hash '" << type_hash << "'";
        bool success = get_type_hashes().insert({typeid(T), type_hash}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
        FieldType<T> field_type{name};
        if constexpr (!std::is_void_v<T>) {
            field_type.assert_complete();
        }
        auto ret = get_field_types().insert({type_hash, field_type.cast()});
        if (!ret.second) {
            if (ret.first->second.name == field_type.name) {
                throw already_registered_type_error(type_hash);
            } else {
                std::ostringstream str;
                str << "hash collision between '" << field_type.name << "' and '" << ret.first->second.name << "'";
                throw std::runtime_error(str.str());
            }
        }
    }

    template<typename T>
    static uint64_t get_type_hash() {
        try {
            return get_type_hashes().at(typeid(T));
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_type_hash() for type '" << typeid(T).name() << "'";
            throw std::runtime_error(str.str());
        }
    }

    template<typename T>
    static const uint64_t* find_type_hash() {
        auto& type_hashes = get_type_hashes();
        auto it = type_hashes.find(typeid(T));
        if (it == type_hashes.end()) {
            return nullptr;
        }
        return &it->second;
    }

    static const std::string& get_type_name(uint64_t type_hash) {
        return get_field_type(type_hash).name;
    }

    template<typename T>
    static const std::string& get_type_name() {
        return get_type_name(get_type_hash<T>());
    }

    static const ConstructorFn<>& get_constructor(uint64_t type_hash) {
        return get_field_type(type_hash).constructor_fn;
    }

    static const DestructorFn<>& get_destructor(uint64_t type_hash) {
        return get_field_type(type_hash).destructor_fn;
    }

    static const SerializeTextFn<>& get_serializer_text(uint64_t type_hash) {
        return get_field_type(type_hash).serialize_text_fn;
    }

    static const SerializeYamlFn<>& get_serializer_yaml(uint64_t type_hash) {
        return get_field_type(type_hash).serialize_yaml_fn;
    }

    static const CheckFn<>& get_check(uint64_t type_hash) {
        return get_field_type(type_hash).check_fn;
    }

    static const CmpEqualFn<>& get_cmp_equal(uint64_t type_hash) {
        return get_field_type(type_hash).cmp_equal_fn;
    }
};

template<>
uint64_t typing::get_type_hash<void>();

template<typename T>
inline void StlAllocator<T>::construct(T* p) {
    const uint64_t* type_hash = typing::find_type_hash<T>();
    if (type_hash) {
        typing::get_constructor (*type_hash)(mm_alloc, p);
    } else {
        new (p) T;
    }
}

using string = std::basic_string<char, std::char_traits<char>, StlAllocator<char>>;

template<class T>
using vector = std::vector<T, StlAllocator<T>>;

template<class K, class T>
using unordered_map = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, StlAllocator<std::pair<const K, T>>>;

template<>
inline YAML::Node to_yaml(const string& str);

template<>
void check(MiniMalloc& mm_alloc, const structstore::string& str);

} // namespace structstore

#endif

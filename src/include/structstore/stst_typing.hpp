#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_utils.hpp"

#include <functional>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <typeindex>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace structstore {

class StructStore;

template<typename T>
class Struct;

extern MiniMalloc static_alloc;

class typing {
public:
    template<typename T>
    class FieldBase {
    private:
	friend class typing;

        static void check_interface() {
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>().to_text(std::cout)), void>);
            static_assert(std::is_same_v<decltype(std::declval<const T>().to_yaml()), YAML::Node>);
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>().check(static_alloc)), void>);
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>() == std::declval<const T>()),
                                   bool>);
            static_assert(
                    std::is_same_v<decltype(std::declval<T>() = std::declval<const T>()), T&>);
        }

    public:
        friend std::ostream& operator<<(std::ostream& os, const FieldBase<T>& t) {
            ((const T&) t).to_text(os);
            return os;
        }

        inline bool operator!=(const T& other) const { return !(*this == other); }
    };

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

    template<typename T = void>
    using CopyFn = std::function<void(MiniMalloc&, T*, const T*)>;

    template<typename T = void>
    struct FieldType_ {
        static_assert(!std::is_class_v<T> || std::is_base_of_v<FieldBase<T>, T>);
        std::string name;
        size_t size;
        ConstructorFn<T> constructor_fn;
        DestructorFn<T> destructor_fn;
        SerializeTextFn<T> serialize_text_fn;
        SerializeYamlFn<T> serialize_yaml_fn;
        CheckFn<T> check_fn;
        CmpEqualFn<T> cmp_equal_fn;
        CopyFn<T> copy_fn;
    };

    using FieldType = FieldType_<>;

private:
    template<typename T>
    static FieldType create_field_type(const std::string name) {
        static_assert(!std::is_void_v<T>);
        FieldType_<T> t;
        t.name = name;
        t.size = sizeof(T);
        if constexpr (std::is_constructible_v<T, MiniMalloc&>) {
            t.constructor_fn = [](MiniMalloc& mm_alloc, T* t) { new (t) T(mm_alloc); };
        } else if constexpr (std::is_constructible_v<T, const StlAllocator<T>&>) {
            t.constructor_fn = [](MiniMalloc& mm_alloc, T* t) {
                new (t) T(StlAllocator<T>{mm_alloc});
            };
        } else {
            t.constructor_fn = [](MiniMalloc&, T* t) { new (t) T(); };
        }
        t.destructor_fn = [](MiniMalloc&, T* t) { t->~T(); };
        t.serialize_text_fn = [](std::ostream& os, const T* t) { os << *t; };
        if constexpr (std::is_class_v<T>) {
            t.serialize_yaml_fn = [](const T* t) -> YAML::Node { return t->to_yaml(); };
            t.check_fn = [](MiniMalloc& mm_alloc, const T* t) { t->check(mm_alloc); };
        } else {
            t.serialize_yaml_fn = [](const T* t) -> YAML::Node { return YAML::Node(*t); };
            t.check_fn = [](MiniMalloc& mm_alloc, const T* t) { mm_alloc.assert_owned(t); };
        }
        t.cmp_equal_fn = [](const T* t, const T* other) { return *t == *other; };
        t.copy_fn = [](MiniMalloc&, T* t, const T* other) { *t = *other; };
        return (FieldType&) t;
    }

    static FieldType create_void_field_type(const std::string name) {
        FieldType_<> t;
        t.name = name;
        t.size = 0;
        t.constructor_fn = [](MiniMalloc&, void*) {};
        t.destructor_fn = [](MiniMalloc&, void*) {};
        t.serialize_text_fn = [](std::ostream& os, const void*) { os << "<empty>"; };
        t.serialize_yaml_fn = [](const void*) { return YAML::Node(YAML::Null); };
        t.check_fn = [](MiniMalloc&, const void* t) {
            if (t != nullptr) {
                throw std::runtime_error("internal error: empty data ptr is not nullptr");
            }
        };
        t.cmp_equal_fn = [](const void*, const void*) { return true; };
        t.copy_fn = [](MiniMalloc&, void*, const void*) {};
        return t;
    }

    static std::unordered_map<std::type_index, uint64_t>& get_type_hashes();

    static std::unordered_map<uint64_t, const FieldType>& get_field_types();

public:
    static void register_common_types();

    static std::runtime_error already_registered_type_error(uint64_t type_hash) {
        std::ostringstream str;
        str << "type already registered: " << get_type(type_hash).name;
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
        if constexpr (std::is_class_v<T>) {
            static_assert(std::is_base_of_v<FieldBase<T>, T>);
            FieldBase<T>::check_interface();
        }
        STST_LOG_DEBUG() << "registering type '" << name << "' with hash '" << type_hash << "'";
        bool success = get_type_hashes().insert({typeid(T), type_hash}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
        FieldType field_type;
        if constexpr (std::is_void_v<T>) {
            field_type = create_void_field_type("<empty>");
        } else {
            field_type = create_field_type<T>(name);
        }
        auto ret = get_field_types().insert({type_hash, field_type});
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

    static const FieldType& get_type(uint64_t type_hash);

    template<typename T>
    inline static const FieldType& get_type() {
        return get_type(get_type_hash<T>());
    }
};

template<>
uint64_t typing::get_type_hash<void>();

template<typename T>
inline void StlAllocator<T>::construct(T* p) {
    const uint64_t* type_hash = typing::find_type_hash<T>();
    if (type_hash) {
        typing::get_type(*type_hash).constructor_fn(mm_alloc, p);
    } else {
        new (p) T;
    }
}

template<class T>
using vector = std::vector<T, StlAllocator<T>>;

template<class K, class T>
using unordered_map = std::unordered_map<K, T, std::hash<K>, std::equal_to<K>, StlAllocator<std::pair<const K, T>>>;

} // namespace structstore

#endif

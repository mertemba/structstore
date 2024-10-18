#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_alloc.hpp"
#include "structstore/stst_utils.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>
#include <typeindex>

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
    static std::unordered_map<std::type_index, uint64_t>& get_type_hashes();

    static std::unordered_map<uint64_t, std::string>& get_type_names();

    static std::unordered_map<uint64_t, ConstructorFn<>>& get_constructors();

    static std::unordered_map<uint64_t, DestructorFn<>>& get_destructors();

    static std::unordered_map<uint64_t, SerializeTextFn<>>& get_serializers_text();

    static std::unordered_map<uint64_t, SerializeYamlFn<>>& get_serializers_yaml();

    static std::unordered_map<uint64_t, CheckFn<>> &get_checks();

    static std::unordered_map<uint64_t, CmpEqualFn<>>& get_cmp_equal_fns();

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
        STST_LOG_DEBUG() << "registering type '" << name << "' with hash '" << type_hash << "'";
        if (type_hash == 0) {
            std::ostringstream str;
            str << "hash collision between '" << name << "' and empty type";
            throw std::runtime_error(str.str());
        }
        bool success = get_type_hashes().insert({typeid(T), type_hash}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
        auto ret = get_type_names().insert({type_hash, name});
        if (!ret.second) {
            if (ret.first->second == name) {
                throw already_registered_type_error(type_hash);
            } else {
                std::ostringstream str;
                str << "hash collision between '" << name << "' and '" << ret.first->second << "'";
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
        try {
            return get_type_names().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_type_name() for hash '" << type_hash << "'";
            throw std::runtime_error(str.str());
        }
    }

    template<typename T>
    static const std::string& get_type_name() {
        return get_type_name(get_type_hash<T>());
    }

    template<typename T>
    static void register_default_constructor() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static ConstructorFn<T> default_constructor = [](MiniMalloc&, T* t) {
            new(t)T();
        };
        bool success = get_constructors().insert(
                {type_hash, (const ConstructorFn<>&) default_constructor}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_mm_alloc_constructor() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static ConstructorFn<T> alloc_constructor = [](MiniMalloc& mm_alloc, T* t) {
            new(t)T(mm_alloc);
        };
        bool success = get_constructors().insert(
                {type_hash, (const ConstructorFn<>&) alloc_constructor}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_stl_alloc_constructor() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static ConstructorFn<T> alloc_constructor = [](MiniMalloc& mm_alloc, T* t) {
            StlAllocator<T> tmp_alloc{mm_alloc};
            new(t)T(tmp_alloc);
        };
        bool success = get_constructors().insert(
                {type_hash, (const ConstructorFn<>&) alloc_constructor}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_default_destructor() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static DestructorFn<T> default_destructor = [](MiniMalloc&, T* t) {
            t->~T();
        };
        bool success = get_destructors().insert(
                {type_hash, (const DestructorFn<>&) default_destructor}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_default_serializer_text() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static SerializeTextFn<T> serializer = [](std::ostream& os, const T* t) -> std::ostream& {
            return to_text(os, *t);
        };
        bool success = get_serializers_text().insert(
                {type_hash, (const SerializeTextFn<>&) serializer}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_dummy_serializer_text() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static SerializeTextFn<T> serializer = [type_hash](std::ostream& os, const T* t) -> std::ostream& {
            return os << "{" << typing::get_type_name(type_hash) << "}";
        };
        bool success = get_serializers_text().insert(
                {type_hash, (const SerializeTextFn<>&) serializer}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_serializer_text(const SerializeTextFn<T>& serializer) {
        uint64_t type_hash = typing::get_type_hash<T>();
        bool success = get_serializers_text().insert(
                {type_hash, (const SerializeTextFn<>&) serializer}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_default_serializer_yaml() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static SerializeYamlFn<T> serializer = [](const T* t) {
            return to_yaml(*t);
        };
        bool success = get_serializers_yaml().insert(
                {type_hash, (const SerializeYamlFn<>&) serializer}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template<typename T>
    static void register_serializer_yaml(const SerializeYamlFn<T>& serializer) {
        uint64_t type_hash = typing::get_type_hash<T>();
        bool success = get_serializers_yaml().insert(
                {type_hash, (const SerializeYamlFn<>&) serializer}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    template <typename T> static void register_default_check() {
      uint64_t type_hash = typing::get_type_hash<T>();
      static CheckFn<T> check = [](MiniMalloc &mm_alloc, const T *t) {
        try_with_info("in default check: ", mm_alloc.assert_owned(t););
      };
      bool success =
          get_checks().insert({type_hash, (const CheckFn<> &)check}).second;
      if (!success) {
        throw already_registered_type_error(type_hash);
      }
    }

    template <typename T> static void register_check(const CheckFn<T> &check) {
      uint64_t type_hash = typing::get_type_hash<T>();
      bool success =
          get_checks().insert({type_hash, (const CheckFn<> &)check}).second;
      if (!success) {
        throw already_registered_type_error(type_hash);
      }
    }

    template<typename T>
    static void register_default_cmp_equal_fn() {
        uint64_t type_hash = typing::get_type_hash<T>();
        static CmpEqualFn<T> default_cmp_equal = [](const T* t, const T* other) {
            return *t == *other;
        };
        bool success = get_cmp_equal_fns().insert({type_hash, (const CmpEqualFn<>&) default_cmp_equal}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
    }

    static ConstructorFn<>& get_constructor(uint64_t type_hash) {
        try {
            return get_constructors().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_constructor() for type '" << get_type_name(type_hash) << "'";
            throw std::runtime_error(str.str());
        }
    }

    static const DestructorFn<>& get_destructor(uint64_t type_hash) {
        try {
            return get_destructors().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_destructor() for type '" << get_type_name(type_hash) << "'";
            throw std::runtime_error(str.str());
        }
    }

    static const SerializeTextFn<>& get_serializer_text(uint64_t type_hash) {
        try {
            return get_serializers_text().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_serializer_text() for type '" << get_type_name(type_hash) << "'";
            throw std::runtime_error(str.str());
        }
    }

    static const SerializeYamlFn<>& get_serializer_yaml(uint64_t type_hash) {
        try {
            return get_serializers_yaml().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_serializer_yaml() for type '" << get_type_name(type_hash) << "'";
            throw std::runtime_error(str.str());
        }
    }

    static const CheckFn<>& get_check(uint64_t type_hash) {
        try {
            return get_checks().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_check() for type '" << get_type_name(type_hash)
                << "'";
            throw std::runtime_error(str.str());
        }
    }

    static const CmpEqualFn<>& get_cmp_equal(uint64_t type_hash) {
        try {
            return get_cmp_equal_fns().at(type_hash);
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_cmp_equal() for type '" << get_type_name(type_hash)
                << "'";
            throw std::runtime_error(str.str());
        }
    }

    template<typename T>
    static void register_basic_type(const char* name) {
        typing::register_type<T>(name);
        typing::register_default_constructor<T>();
        typing::register_default_destructor<T>();
        typing::register_default_serializer_text<T>();
        typing::register_default_serializer_yaml<T>();
        typing::register_default_check<T>();
        typing::register_default_cmp_equal_fn<T>();
    }

};

template<>
uint64_t typing::get_type_hash<void>();

template<typename T>
inline void StlAllocator<T>::construct(T* p) {
    const uint64_t* type_hash = typing::find_type_hash<T>();
    if (type_hash) {
    typing::get_constructor(*type_hash)(mm_alloc, p);
    } else {
        new (p) T;
    }
}

} // namespace structstore

#include <structstore/stst_stl_types.hpp>

#endif

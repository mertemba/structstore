#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_alloc.hpp"

#include <iostream>
#include <unordered_map>
#include <functional>
#include <typeindex>

#include <yaml-cpp/yaml.h>

namespace structstore {

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

private:

    static std::unordered_map<std::type_index, uint64_t>& get_type_hashes();

    static std::unordered_map<uint64_t, std::string>& get_type_names();

    static std::unordered_map<uint64_t, ConstructorFn<>>& get_constructors();

    static std::unordered_map<uint64_t, DestructorFn<>>& get_destructors();

    static std::unordered_map<uint64_t, SerializeTextFn<>>& get_serializers_text();

    static std::unordered_map<uint64_t, SerializeYamlFn<>>& get_serializers_yaml();

public:

    static void register_common_types();

    template<typename T>
    static void register_type(const char* name) {
        uint64_t type_hash = const_hash(name);
        bool success = get_type_hashes().insert({typeid(T), type_hash}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
        auto ret = get_type_names().insert({type_hash, name});
        if (!ret.second) {
            if (ret.first->second == name) {
                throw std::runtime_error("type already registered");
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
    static void register_default_constructor() {
        static ConstructorFn<T> default_constructor = [](MiniMalloc&, T* t) {
            new(t)T();
        };
        bool success = get_constructors().insert(
                {get_type_hash<T>(), (const ConstructorFn<>&) default_constructor}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_mm_alloc_constructor() {
        static ConstructorFn<T> alloc_constructor = [](MiniMalloc& mm_alloc, T* t) {
            new(t)T(mm_alloc);
        };
        bool success = get_constructors().insert(
                {get_type_hash<T>(), (const ConstructorFn<>&) alloc_constructor}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_stl_alloc_constructor() {
        static ConstructorFn<T> alloc_constructor = [](MiniMalloc& mm_alloc, T* t) {
            StlAllocator<T> tmp_alloc{mm_alloc};
            new(t)T(tmp_alloc);
        };
        bool success = get_constructors().insert(
                {get_type_hash<T>(), (const ConstructorFn<>&) alloc_constructor}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_default_destructor() {
        static DestructorFn<T> default_destructor = [](MiniMalloc&, T* t) {
            t->~T();
        };
        bool success = get_destructors().insert(
                {get_type_hash<T>(), (const DestructorFn<>&) default_destructor}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_default_serializer_text() {
        static SerializeTextFn<T> serializer = [](std::ostream& os, const T* t) -> std::ostream& {
            return os << *t;
        };
        bool success = get_serializers_text().insert(
                {get_type_hash<T>(), (const SerializeTextFn<>&) serializer}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_serializer_text(const SerializeTextFn<T>& serializer) {
        bool success = get_serializers_text().insert(
                {get_type_hash<T>(), (const SerializeTextFn<>&) serializer}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_default_serializer_yaml() {
        static SerializeYamlFn<T> serializer = [](const T* t) {
            return YAML::Node(*t);
        };
        bool success = get_serializers_yaml().insert(
                {get_type_hash<T>(), (const SerializeYamlFn<>&) serializer}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
        }
    }

    template<typename T>
    static void register_serializer_yaml(const SerializeYamlFn<T>& serializer) {
        bool success = get_serializers_yaml().insert(
                {get_type_hash<T>(), (const SerializeYamlFn<>&) serializer}).second;
        if (!success) {
            throw std::runtime_error("type already registered");
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

    template<typename T>
    static void register_basic_type(const char* name) {
        typing::register_type<T>(name);
        typing::register_default_constructor<T>();
        typing::register_default_destructor<T>();
        typing::register_default_serializer_text<T>();
        typing::register_default_serializer_yaml<T>();
    }

};

}

#endif

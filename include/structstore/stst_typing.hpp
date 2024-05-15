#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_alloc.hpp"

#include <iostream>
#include <unordered_map>
#include <typeindex>
#include <functional>

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

    static std::unordered_map<std::type_index, ConstructorFn<>>& get_constructors();

    static std::unordered_map<std::type_index, DestructorFn<>>& get_destructors();

    static std::unordered_map<std::type_index, SerializeTextFn<>>& get_serializers_text();

    static std::unordered_map<std::type_index, SerializeYamlFn<>>& get_serializers_yaml();

    static bool registered_common_types;

public:
    template<typename T>
    static bool register_default_constructor() {
        static ConstructorFn<T> default_constructor = [](MiniMalloc&, T* t) {
            new(t)T();
        };
        get_constructors().insert({typeid(T), (const ConstructorFn<>&) default_constructor});
        return true;
    }

    template<typename T>
    static bool register_mm_alloc_constructor() {
        static ConstructorFn<T> alloc_constructor = [](MiniMalloc& mm_alloc, T* t) {
            new(t)T(mm_alloc);
        };
        get_constructors().insert({typeid(T), (const ConstructorFn<>&) alloc_constructor});
        return true;
    }

    template<typename T>
    static bool register_stl_alloc_constructor() {
        static ConstructorFn<T> alloc_constructor = [](MiniMalloc& mm_alloc, T* t) {
            StlAllocator<T> tmp_alloc{mm_alloc};
            new(t)T(tmp_alloc);
        };
        get_constructors().insert({typeid(T), (const ConstructorFn<>&) alloc_constructor});
        return true;
    }

    template<typename T>
    static bool register_default_destructor() {
        static DestructorFn<T> default_destructor = [](MiniMalloc&, T* t) {
            t->~T();
        };
        get_destructors().insert({typeid(T), (const DestructorFn<>&) default_destructor});
        return true;
    }

    template<typename T>
    static bool register_default_serializer_text() {
        static SerializeTextFn<T> serializer = [](std::ostream& os, const T* t) -> std::ostream& {
            return os << *t;
        };
        get_serializers_text().insert({typeid(T), (const SerializeTextFn<>&) serializer});
        return true;
    }

    template<typename T>
    static bool register_serializer_text(const SerializeTextFn<T>& serializer) {
        get_serializers_text().insert({typeid(T), (const SerializeTextFn<>&) serializer});
        return true;
    }

    template<typename T>
    static bool register_default_serializer_yaml() {
        static SerializeYamlFn<T> serializer = [](const T* t) {
            return YAML::Node(*t);
        };
        get_serializers_yaml().insert({typeid(T), (const SerializeYamlFn<>&) serializer});
        return true;
    }

    template<typename T>
    static bool register_serializer_yaml(const SerializeYamlFn<T>& serializer) {
        get_serializers_yaml().insert({typeid(T), (const SerializeYamlFn<>&) serializer});
        return true;
    }

    static ConstructorFn<>& get_constructor(std::type_index type) {
        return get_constructors().at(type);
    }

    static const DestructorFn<>& get_destructor(std::type_index type) {
        return get_destructors().at(type);
    }

    static const SerializeTextFn<>& get_serializer_text(std::type_index type) {
        return get_serializers_text().at(type);
    }

    static const SerializeYamlFn<>& get_serializer_yaml(std::type_index type) {
        return get_serializers_yaml().at(type);
    }

};

}

#endif

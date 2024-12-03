#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_lock.hpp"
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

class FieldTypeBase {
protected:
    template<bool write>
    friend class ScopedFieldLock;
    friend class py;

    const FieldTypeBase* parent_field = nullptr;
    mutable SpinMutex mutex = {};

    FieldTypeBase() {}
    FieldTypeBase(const FieldTypeBase&) {}
    FieldTypeBase(FieldTypeBase&&) {}
    FieldTypeBase& operator=(const FieldTypeBase&) { return *this; }
    FieldTypeBase& operator=(FieldTypeBase&&) { return *this; }

    void read_lock_() const;
    void read_unlock_() const;
    void write_lock_() const;
    void write_unlock_() const;

public:
    [[nodiscard]] ScopedFieldLock<false> read_lock() const { return ScopedFieldLock<false>(*this); }

    [[nodiscard]] ScopedFieldLock<true> write_lock() const { return ScopedFieldLock<true>(*this); }
};

class typing {
public:
    template<typename T>
    class FieldType : public FieldTypeBase {
    private:
        friend class typing;

        static void check_interface() {
            static_assert(std::is_same_v<decltype(T::type_info), const TypeInfo&>);
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>().to_text(std::cout)), void>);
            static_assert(std::is_same_v<decltype(std::declval<const T>().to_yaml()), YAML::Node>);
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>().check(
                                           static_alloc, std::declval<const FieldTypeBase*>())),
                                   void>);
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>() == std::declval<const T>()),
                                   bool>);
            static_assert(
                    std::is_same_v<decltype(std::declval<T>() = std::declval<const T>()), T&>);
        }

    public:
        friend std::ostream& operator<<(std::ostream& os, const FieldType<T>& t) {
            ((const T&) t).to_text(os);
            return os;
        }

        inline bool operator!=(const T& other) const { return !((const T&) *this == other); }
    };

    template<typename T>
    constexpr static bool is_field_type = std::is_base_of_v<FieldType<T>, T> || !std::is_class_v<T>;

    using ConstructorFn = std::function<void(MiniMalloc&, void*, const FieldTypeBase*)>;

    using DestructorFn = std::function<void(MiniMalloc&, void*)>;

    using SerializeTextFn = std::function<void(std::ostream&, const void*)>;

    using SerializeYamlFn = std::function<YAML::Node(const void*)>;

    using CheckFn = std::function<void(const MiniMalloc&, const void*, const FieldTypeBase*)>;

    using CmpEqualFn = std::function<bool(const void*, const void*)>;

    using CopyFn = std::function<void(MiniMalloc&, void*, const void*)>;

    struct TypeInfo {
        uint64_t type_hash;
        std::string name;
        size_t size;
        ConstructorFn constructor_fn;
        DestructorFn destructor_fn;
        SerializeTextFn serialize_text_fn;
        SerializeYamlFn serialize_yaml_fn;
        CheckFn check_fn;
        CmpEqualFn cmp_equal_fn;
        CopyFn copy_fn;
    };

private:
    template<typename T>
    static TypeInfo create_type_info(const std::string name) {
        static_assert(!std::is_void_v<T>);
        TypeInfo t;
        t.type_hash = -1;
        t.name = name;
        t.size = sizeof(T);
        if constexpr (std::is_constructible_v<T, MiniMalloc&>) {
            t.constructor_fn = [](MiniMalloc& mm_alloc, void* t,
                                  const FieldTypeBase* parent_field) {
                new (t) T(mm_alloc);
                ((T*) t)->parent_field = parent_field;
            };
        } else if constexpr (std::is_constructible_v<T, const StlAllocator<T>&>) {
            t.constructor_fn = [](MiniMalloc& mm_alloc, void* t,
                                  const FieldTypeBase* parent_field) {
                new (t) T(StlAllocator<T>{mm_alloc});
                ((T*) t)->parent_field = parent_field;
            };
        } else {
            t.constructor_fn = [](MiniMalloc&, void* t, const FieldTypeBase* parent_field) {
                new (t) T();
                if constexpr (std::is_class_v<T>) { ((T*) t)->parent_field = parent_field; }
            };
        }
        t.destructor_fn = [](MiniMalloc&, void* t) { ((T*) t)->~T(); };
        t.serialize_text_fn = [](std::ostream& os, const void* t) { os << *(const T*) t; };
        if constexpr (std::is_class_v<T>) {
            t.serialize_yaml_fn = [](const void* t) -> YAML::Node {
                return ((const T*) t)->to_yaml();
            };
            t.check_fn = [](const MiniMalloc& mm_alloc, const void* t,
                            const FieldTypeBase* parent_field) {
                if (((const T*) t)->parent_field != parent_field) {
                    throw std::runtime_error("invalid parent_field pointer in field of type " +
                                             T::type_info.name);
                }
                mm_alloc.assert_owned((const T*) t);
                ((const T*) t)->check(mm_alloc, parent_field);
            };
        } else {
            t.serialize_yaml_fn = [](const void* t) -> YAML::Node {
                return YAML::Node(*(const T*) t);
            };
            t.check_fn = [](const MiniMalloc& mm_alloc, const void* t, const FieldTypeBase*) {
                mm_alloc.assert_owned((const T*) t);
            };
        }
        t.cmp_equal_fn = [](const void* t, const void* other) {
            return *(const T*) t == *(const T*) other;
        };
        t.copy_fn = [](MiniMalloc&, void* t, const void* other) { *(T*) t = *(const T*) other; };
        return t;
    }

    template<typename T>
    static TypeInfo create_ptr_type_info(const std::string name) {
        static_assert(std::is_pointer_v<T>);
        TypeInfo ti;
        ti.type_hash = -1;
        ti.name = name;
        ti.size = sizeof(T);
        ti.constructor_fn = [](MiniMalloc&, void* t, const FieldTypeBase*) { *(T**) t = nullptr; };
        ti.destructor_fn = [](MiniMalloc&, void* t) { *(T**) t = nullptr; };
        ti.serialize_text_fn = [name](std::ostream& os, const void*) { os << "<" << name << ">"; };
        ti.serialize_yaml_fn = [=](const void*) -> YAML::Node {
            return YAML::Node{"<" + name + ">"};
        };
        ti.check_fn = [=](const MiniMalloc& mm_alloc, const void* t, const FieldTypeBase*) {
            mm_alloc.assert_owned(t);
            if (*(T**) t != nullptr) { mm_alloc.assert_owned(*(T* const*) t); }
        };
        ti.cmp_equal_fn = [=](const void* t, const void* other) {
            return *(T* const*) t == *(T* const*) other || **(T* const*) t == **(T* const*) other;
        };
        ti.copy_fn = [=](MiniMalloc&, void* t, const void* other) {
            *(T**) t = *(T* const*) other;
        };
        return ti;
    }

    static TypeInfo create_void_type_info(const std::string name) {
        TypeInfo t;
        t.name = name;
        t.size = 0;
        t.constructor_fn = [](MiniMalloc&, void*, const FieldTypeBase*) {};
        t.destructor_fn = [](MiniMalloc&, void*) {};
        t.serialize_text_fn = [](std::ostream& os, const void*) { os << "<empty>"; };
        t.serialize_yaml_fn = [](const void*) { return YAML::Node(YAML::Null); };
        t.check_fn = [](const MiniMalloc&, const void* t, const FieldTypeBase*) {
            if (t != nullptr) {
                throw std::runtime_error("internal error: empty data ptr is not nullptr");
            }
        };
        t.cmp_equal_fn = [](const void*, const void*) { return true; };
        t.copy_fn = [](MiniMalloc&, void*, const void*) {};
        return t;
    }

    static std::unordered_map<std::type_index, uint64_t>& get_type_hashes();

    static std::unordered_map<uint64_t, const TypeInfo>& get_type_infos();

    template<typename T>
    static const TypeInfo& register_type_internal(const std::string& name) {
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
            static_assert(std::is_base_of_v<FieldType<T>, T>);
            FieldType<T>::check_interface();
        }
        STST_LOG_DEBUG() << "registering type '" << name << "' with hash '" << type_hash << "'";
        bool success = get_type_hashes().insert({typeid(T), type_hash}).second;
        if (!success) {
            throw already_registered_type_error(type_hash);
        }
        TypeInfo type_info;
        if constexpr (std::is_void_v<T>) {
            type_info = create_void_type_info("<empty>");
        } else if constexpr (std::is_pointer_v<T>) {
            type_info = create_ptr_type_info<T>(name);
        } else {
            type_info = create_type_info<T>(name);
        }
        type_info.type_hash = type_hash;
        auto ret = get_type_infos().insert({type_hash, type_info});
        if (!ret.second) {
            if (ret.first->second.name == type_info.name) {
                throw already_registered_type_error(type_hash);
            } else {
                std::ostringstream str;
                str << "hash collision between '" << type_info.name << "' and '" << ret.first->second.name << "'";
                throw std::runtime_error(str.str());
            }
        }
        if constexpr (!std::is_pointer_v<T>) {
            // register corresponding pointer type
            register_type_internal<T*>(name + '*');
        }
        // this triggers a `-Wdangling-reference` at the call site in GCC
        const TypeInfo& inserted_type_info = ret.first->second;
        return inserted_type_info;
    }

public:
    static std::runtime_error already_registered_type_error(uint64_t type_hash) {
        std::ostringstream str;
        str << "type already registered: " << get_type(type_hash).name;
        return std::runtime_error(str.str());
    }

    template<typename T>
    static const TypeInfo& register_type(const std::string& name) {
        return register_type_internal<T>(name);
    }

    template<typename T>
    static uint64_t get_type_hash() {
        if constexpr (std::is_class_v<T>) {
            static_assert(std::is_base_of_v<FieldType<T>, T>);
            return T::type_info.type_hash;
        } else if constexpr (std::is_void_v<T>) {
            return 0;
        }
        try {
            return get_type_hashes().at(typeid(T));
        } catch (const std::out_of_range&) {
            std::ostringstream str;
            str << "error at get_type_hash() for type '" << typeid(T).name() << "'";
            throw std::runtime_error(str.str());
        }
    }

    static const TypeInfo& get_type(uint64_t type_hash);

    template<typename T>
    inline static const TypeInfo& get_type() {
        static_assert(is_field_type<T>);
        if constexpr (std::is_class_v<T>) { return T::type_info; }
        return get_type(get_type_hash<T>());
    }
};

using TypeInfo = typing::TypeInfo;
template<typename T>
using FieldType = typing::FieldType<T>;

template<typename T>
inline void StlAllocator<T>::construct(T* p) {
    if constexpr (std::is_base_of_v<FieldType<T>, T>) {
        T::type_info.constructor_fn(mm_alloc, p);
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

#ifndef STST_TYPING_HPP
#define STST_TYPING_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_callstack.hpp"
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

using type_hash_t = uint32_t;

template<typename T>
class Struct;

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class FieldTypeBase {
protected:
    template<bool write>
    friend class ScopedFieldLock;
    friend class py;

    mutable SpinMutex mutex = {};
    OffsetPtr<const FieldTypeBase> parent_field = nullptr;

    FieldTypeBase() {}
    FieldTypeBase(const FieldTypeBase&) {}
    FieldTypeBase(FieldTypeBase&&) {}
    FieldTypeBase& operator=(const FieldTypeBase&) { return *this; }
    FieldTypeBase& operator=(FieldTypeBase&&) { return *this; }

    void read_lock_() const;
    void read_unlock_() const;
    void write_lock_() const;
    void write_unlock_() const;
    void read_or_write_lock_() const;
    void read_or_write_unlock_() const;

public:
    [[nodiscard]] ScopedFieldLock<false> read_lock() const { return ScopedFieldLock<false>(*this); }

    [[nodiscard]] ScopedFieldLock<true> write_lock() const { return ScopedFieldLock<true>(*this); }
};

template<typename T>
class FieldRef;

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
            static_assert(std::is_same_v<decltype(std::declval<const T>().check(
                                                 std::declval<const SharedAlloc*>())),
                                         void>);
            static_assert(
                    std::is_same_v<decltype(std::declval<const T>() == std::declval<const T>()),
                                   bool>);
            static_assert(
                    std::is_same_v<decltype(std::declval<T>() = std::declval<const T>()), T&>);
        }

    public:
        using Ref = FieldRef<T>;

        static FieldRef<T> create() { return FieldRef<T>::create(); }

        friend std::ostream& operator<<(std::ostream& os, const FieldType<T>& t) {
            ((const T&) t).to_text(os);
            return os;
        }

        inline bool operator!=(const T& other) const { return !((const T&) *this == other); }
    };

    template<typename T>
    constexpr static bool is_field_type =
            !std::is_pointer_v<T> && (std::is_base_of_v<FieldType<T>, T> ||
                                      std::is_base_of_v<OffsetPtrBase<>, T> || !std::is_class_v<T>);

    using ConstructorFn = std::function<void(SharedAlloc&, void*, const FieldTypeBase*)>;

    using DestructorFn = std::function<void(SharedAlloc&, void*)>;

    using SerializeTextFn = std::function<void(std::ostream&, const void*)>;

    using SerializeYamlFn = std::function<YAML::Node(const void*)>;

    using CheckFn = std::function<void(const SharedAlloc&, const void*, const FieldTypeBase&)>;

    using CmpEqualFn = std::function<bool(const void*, const void*)>;

    using CopyFn = std::function<void(SharedAlloc&, void*, const void*)>;

    struct TypeInfo {
        type_hash_t type_hash;
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
        static_assert(!std::is_pointer_v<T>);
        TypeInfo ti;
        ti.type_hash = -1;
        ti.name = name;
        ti.size = sizeof(T);
        if constexpr (std::is_constructible_v<T, SharedAlloc&>) {
            ti.constructor_fn = [](SharedAlloc& sh_alloc, void* t,
                                   const FieldTypeBase* parent_field) {
                new (t) T(sh_alloc);
                ((T*) t)->parent_field = parent_field;
            };
        } else if constexpr (std::is_constructible_v<T, const StlAllocator<T>&>) {
            ti.constructor_fn = [](SharedAlloc& sh_alloc, void* t,
                                   const FieldTypeBase* parent_field) {
                new (t) T(StlAllocator<T>{sh_alloc});
                ((T*) t)->parent_field = parent_field;
            };
        } else {
            ti.constructor_fn = [](SharedAlloc&, void* t, const FieldTypeBase* parent_field) {
                new (t) T();
                if constexpr (std::is_class_v<T>) { ((T*) t)->parent_field = parent_field; }
            };
        }
        ti.destructor_fn = [](SharedAlloc&, void* t) { ((T*) t)->~T(); };
        ti.serialize_text_fn = [](std::ostream& os, const void* t) { os << *(const T*) t; };
        if constexpr (std::is_class_v<T>) {
            ti.serialize_yaml_fn = [](const void* t) -> YAML::Node {
                return ((const T*) t)->to_yaml();
            };
            ti.check_fn = [](const SharedAlloc& sh_alloc, const void* t,
                             const FieldTypeBase& parent_field) {
                CallstackEntry entry{"structstore::typing::check()"};
                stst_assert(sh_alloc.is_owned(t));
                stst_assert(((const T*) t)->parent_field == &parent_field);
                ((const T*) t)->check(&sh_alloc);
            };
        } else {
            ti.serialize_yaml_fn = [](const void* t) -> YAML::Node {
                return YAML::Node(*(const T*) t);
            };
            ti.check_fn = [](const SharedAlloc& sh_alloc, const void* t, const FieldTypeBase&) {
                CallstackEntry entry{"structstore::typing::check()"};
                stst_assert(sh_alloc.is_owned(t));
            };
        }
        ti.cmp_equal_fn = [](const void* t, const void* other) {
            return *(const T*) t == *(const T*) other;
        };
        ti.copy_fn = [](SharedAlloc&, void* t, const void* other) { *(T*) t = *(const T*) other; };
        return ti;
    }

    template<typename T>
    static TypeInfo create_ptr_type_info(const std::string name) {
        static_assert(!std::is_pointer_v<T>);
        static_assert(std::is_base_of_v<OffsetPtrBase<>, T>);
        TypeInfo ti;
        ti.type_hash = -1;
        ti.name = name;
        ti.size = sizeof(T);
        ti.constructor_fn = [](SharedAlloc&, void* t, const FieldTypeBase*) { *(T*) t = nullptr; };
        ti.destructor_fn = [](SharedAlloc&, void* t) { *(T*) t = nullptr; };
        ti.serialize_text_fn = [name](std::ostream& os, const void*) { os << "<" << name << ">"; };
        ti.serialize_yaml_fn = [=](const void*) -> YAML::Node {
            return YAML::Node{"<" + name + ">"};
        };
        ti.check_fn = [](const SharedAlloc& sh_alloc, const void* t, const FieldTypeBase&) {
            CallstackEntry entry{"structstore::typing::check()"};
            stst_assert(sh_alloc.is_owned(t));
            if (*(const T*) t != nullptr) { stst_assert(sh_alloc.is_owned(((const T*) t)->get())); }
        };
        ti.cmp_equal_fn = [](const void* t, const void* other) {
            return *(const T*) t == *(const T*) other || **(const T*) t == **(const T*) other;
        };
        ti.copy_fn = [](SharedAlloc&, void* t, const void* other) { *(T*) t = *(const T*) other; };
        return ti;
    }

    static TypeInfo create_void_type_info(const std::string name) {
        TypeInfo t;
        t.name = name;
        t.size = 0;
        t.constructor_fn = [](SharedAlloc&, void*, const FieldTypeBase*) {};
        t.destructor_fn = [](SharedAlloc&, void*) {};
        t.serialize_text_fn = [](std::ostream& os, const void*) { os << "<empty>"; };
        t.serialize_yaml_fn = [](const void*) { return YAML::Node(YAML::Null); };
        t.check_fn = [](const SharedAlloc&, const void* t, const FieldTypeBase&) {
            if (t != nullptr) {
                throw std::runtime_error("internal error: empty data ptr is not nullptr");
            }
        };
        t.cmp_equal_fn = [](const void*, const void*) { return true; };
        t.copy_fn = [](SharedAlloc&, void*, const void*) {};
        return t;
    }

    static std::unordered_map<std::type_index, type_hash_t>& get_type_hashes();

    static std::unordered_map<type_hash_t, const TypeInfo>& get_type_infos();

    template<typename T>
    static const TypeInfo& register_type_internal(const std::string& name) {
        type_hash_t type_hash = const_hash(name.c_str());
        if constexpr (std::is_void_v<T>) {
            type_hash = 0;
        } else {
            if (type_hash == 0) {
                std::ostringstream str;
                str << "hash collision between '" << name << "' and empty type";
                throw std::runtime_error(str.str());
            }
        }
        if constexpr (std::is_class_v<T> && !std::is_base_of_v<OffsetPtrBase<>, T>) {
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
            type_info = create_void_type_info("void");
        } else if constexpr (std::is_base_of_v<OffsetPtrBase<>, T>) {
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
        if constexpr (!std::is_base_of_v<OffsetPtrBase<>, T> && !std::is_void_v<T>) {
            // register corresponding pointer type
            register_type_internal<OffsetPtr<T>>(name + "_ptr");
        }
        // this triggers a `-Wdangling-reference` at the call site in GCC
        const TypeInfo& inserted_type_info = ret.first->second;
        return inserted_type_info;
    }

public:
    static std::runtime_error already_registered_type_error(type_hash_t type_hash) {
        std::ostringstream str;
        str << "type already registered: " << get_type(type_hash).name;
        return std::runtime_error(str.str());
    }

    template<typename T>
    static const TypeInfo& register_type(const std::string& name) {
        static_assert(typing::is_field_type<T>);
        return register_type_internal<T>(name);
    }

    template<typename T>
    static type_hash_t get_type_hash() {
        static_assert(typing::is_field_type<T>);
        if constexpr (std::is_class_v<T> && !std::is_base_of_v<OffsetPtrBase<>, T>) {
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

    static const TypeInfo& get_type(type_hash_t type_hash);

    template<typename T>
    inline static const TypeInfo& get_type() {
        static_assert(is_field_type<T>);
        if constexpr (std::is_base_of_v<FieldType<T>, T>) { return T::type_info; }
        return get_type(get_type_hash<T>());
    }
};

using TypeInfo = typing::TypeInfo;
template<typename T>
using FieldType = typing::FieldType<T>;

} // namespace structstore

#endif

#ifndef STST_STRUCTSTORE_HPP
#define STST_STRUCTSTORE_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_lock.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_typing.hpp"

#include <iostream>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace nanobind {
template<class type_, class ... options>
struct class_;

struct module_;

struct object;
}

namespace structstore {

extern MiniMalloc static_alloc;

class FieldAccess {
    StructStoreField& field;
    MiniMalloc& mm_alloc;

public:
    FieldAccess() = delete;

    FieldAccess(StructStoreField& field, MiniMalloc& mm_alloc) : field(field), mm_alloc(mm_alloc) {}

    FieldAccess(const FieldAccess& other) = default;

    FieldAccess& operator=(const FieldAccess& other) = delete;

    template<typename T>
    T& get() {
        if (!field.empty()) {
            // field already exists, directly return
            return field.get<T>();
        }
        StlAllocator<T> tmp_alloc{mm_alloc};
        void* ptr = tmp_alloc.allocate(1);
        const typing::ConstructorFn<>& constructor = typing::get_constructor(typeid(T));
        constructor(mm_alloc, ptr);
        field.replace_data<T>(ptr, mm_alloc);
        return field.get<T>();
    }

    ::structstore::string& get_str() {
        return get<::structstore::string>();
    }

    template<typename T>
    operator T&() {
        return get<T>();
    }

    template<typename T>
    FieldAccess& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    StructStoreField& get_field() {
        return field;
    }

    friend std::ostream& operator<<(std::ostream& os, const FieldAccess& self) {
        return os << self.field;
    }

    [[nodiscard]] std::type_index get_type() const {
        return field.get_type();
    }

    void clear() {
        field.clear(mm_alloc);
    }

    template<typename T>
    void set_type() {
        if (field.get_type() != typeid(T)) {
            clear();
            get<T>();
        }
    }
};

class StructStore {
    friend class StructStoreShared;

    friend class FieldAccess;

    friend class List;

public:
    static bool has_constructor;
    static bool has_destructor;
    static bool has_serializer_text;
    static bool has_serializer_yaml;

    MiniMalloc& mm_alloc;
    StlAllocator<char> alloc;
    mutable SpinMutex mutex;

private:
    unordered_map<HashString, StructStoreField> fields;
    vector<HashString> slots;

    HashString internal_string(HashString str) {
        size_t len = std::strlen(str.str);
        char* buf = (char*) mm_alloc.allocate(len + 1);
        std::strcpy(buf, str.str);
        return {buf, str.hash};
    }

public:
    explicit StructStore(MiniMalloc& mm_alloc)
            : mm_alloc(mm_alloc), alloc(mm_alloc), fields(alloc), slots(alloc) {}

    StructStore() : mm_alloc(static_alloc), alloc(mm_alloc), fields(alloc), slots(alloc) {}

    StructStore(const StructStore&) = delete;

    StructStore(StructStore&&) = delete;

    StructStore& operator=(const StructStore&) = delete;

    StructStore& operator=(StructStore&&) = delete;

    ~StructStore() {
        clear();
    }

    void clear() {
        for (auto& [key, value]: fields) {
            value.clear(mm_alloc);
        }
        fields.clear();
        for (const HashString& str: slots) {
            mm_alloc.deallocate((void*) str.str);
        }
        slots.clear();
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStore& self);

    friend YAML::Node to_yaml(const StructStore& self);

    template<bool recursive>
    friend nanobind::object to_object(const StructStore& store);

    StructStoreField* try_get_field(HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            return nullptr;
        }
        return &it->second;
    }

    template<typename T>
    T& get(const char* name) {
        return (*this)[HashString{name}];
    }

    template<typename T>
    T& get_hashed(HashString name) {
        return (*this)[name];
    }

    StructStore& substore(const char* name) {
        return get<StructStore>(name);
    }

    FieldAccess operator[](HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            HashString name_int = internal_string(name);
            it = fields.emplace(name_int, StructStoreField{}).first;
            slots.emplace_back(name_int);
        }
        return {it->second, mm_alloc};
    }

    FieldAccess operator[](const char* name) {
        return operator[](HashString{name});
    }

    FieldAccess at(HashString name) {
        return {fields.at(name), mm_alloc};
    }

    FieldAccess at(const char* name) {
        return at(HashString{name});
    }

    SpinMutex& get_mutex() {
        return mutex;
    }

    [[nodiscard]] auto write_lock() {
        return ScopedLock(mutex);
    }

    [[nodiscard]] auto read_lock() const {
        return ScopedLock(mutex);
    }

    const vector<HashString>& get_slots() const {
        return slots;
    }
};

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value);

template<>
FieldAccess& FieldAccess::operator=<std::string>(const std::string& value);

}

#endif

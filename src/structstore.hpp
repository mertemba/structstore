#ifndef STRUCTSTORE_HPP
#define STRUCTSTORE_HPP

#include <iostream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "serialization.hpp"
#include "structstore_alloc.hpp"
#include "structstore_field.hpp"
#include "hashstring.hpp"

namespace pybind11 {
template<class type_, class ... options>
struct class_;

struct module_;

struct object;
}

namespace structstore {

static constexpr size_t malloc_size = 1 << 16;
static MiniMalloc static_alloc{malloc_size, malloc(malloc_size)};

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
        if (field.get_type() != FieldTypeValue::EMPTY) {
            // field already exists, directly return
            return field.get<T>();
        }
        StlAllocator<T> tmp_alloc{mm_alloc};
        T* ptr = tmp_alloc.allocate(1);
        if constexpr (std::is_base_of<StructStore, T>::value) {
            new(ptr) T(mm_alloc);
        } else if constexpr (std::is_same<T, structstore::string>::value) {
            new(ptr) T(tmp_alloc);
        } else {
            new(ptr) T();
        }
        field.replace_data(ptr, mm_alloc);
        return field.get<T>();
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

    friend std::ostream& operator<<(std::ostream& os, const FieldAccess& self) {
        return os << self.field;
    }
};

class StructStore {
    template<typename T>
    friend pybind11::class_ <T> register_pystruct(pybind11::module_&, const char*);

    friend class StructStoreShared;

    friend class FieldAccess;

protected:
    MiniMalloc& mm_alloc;
    StlAllocator<char> alloc;

private:
    unordered_map<HashString, StructStoreField> fields;
    vector<const char*> slots;

    HashString internal_string(HashString str) {
        size_t len = std::strlen(str.str);
        char* buf = (char*) mm_alloc.allocate(len + 1);
        std::strcpy(buf, str.str);
        return {buf, str.hash};
    }

protected:
    explicit StructStore(MiniMalloc& mm_alloc)
            : mm_alloc(mm_alloc), alloc(mm_alloc), fields(alloc), slots(alloc) {}

public:
    StructStore() : mm_alloc(static_alloc), alloc(mm_alloc), fields(alloc), slots(alloc) {}

    StructStore(const StructStore&) = delete;

    StructStore(StructStore&&) = delete;

    StructStore& operator=(const StructStore&) = delete;

    StructStore& operator=(StructStore&&) = delete;

    size_t allocated_size() const {
        return mm_alloc.allocated;
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStore& self) {
        os << "{";
        for (const auto& [key, value]: self.fields) {
            os << '"' << key.str << "\":" << value << ",";
        }
        os << "}";
        return os;
    }

    friend YAML::Node to_yaml(const StructStore& self) {
        YAML::Node root;
        for (const auto& [key, value]: self.fields) {
            root[key.str] = to_yaml(value);
        }
        return root;
    }

    friend pybind11::object to_dict(StructStore& store);

    StructStoreField& get_field(HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            HashString name_int = internal_string(name);
            it = fields.emplace(name_int, StructStoreField{}).first;
            slots.emplace_back(name_int.str);
        }
        return it->second;
    }

    template<typename T>
    T& get(const char* name) {
        return (*this)[HashString{name}];
    }

    template<typename T>
    T& get_hashed(HashString name) {
        return (*this)[name];
    }

    FieldAccess operator[](HashString name) {
        return {get_field(name), mm_alloc};
    }

    FieldAccess operator[](const char* name) {
        return {get_field(HashString{name}), mm_alloc};
    }
};

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value) {
    structstore::string* str = StlAllocator<structstore::string>(mm_alloc).allocate(1);
    new(str) structstore::string(value, StlAllocator<char>(mm_alloc));
    field.replace_data(str, mm_alloc);
    return *this;
}

}

#endif

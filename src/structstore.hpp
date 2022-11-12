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

class StructStoreAccess {
    StructStore& store;
    HashString name;

public:
    StructStoreAccess() = delete;

    StructStoreAccess(StructStore& store, HashString name) : store(store), name(name) {}

    StructStoreAccess(const StructStoreAccess& other) = default;

    StructStoreAccess& operator=(const StructStoreAccess& other) = delete;

    template<typename T>
    T& get();

    template<typename T>
    operator T&() {
        return get<T>();
    }

    template<typename T>
    StructStoreAccess& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStoreAccess& self);
};

class StructStore {
    template<typename T>
    friend pybind11::class_ <T> register_pystruct(pybind11::module_&, const char*);

    friend class StructStoreShared;

private:
    std::unique_ptr<CompactArena<1024>> own_arena;

protected:
    Arena& arena;
    ArenaAllocator<char> alloc;

private:
    unordered_map<HashString, StructStoreField> fields;
    vector<const char*> slots;

protected:
    explicit StructStore(Arena& arena)
            : own_arena(), arena(arena), alloc(arena), fields(alloc), slots(alloc) {}

    HashString internal_string(HashString str) {
        size_t len = std::strlen(str.str);
        char* buf = (char*) arena.allocate(len + 1);
        std::strcpy(buf, str.str);
        return {buf, str.hash};
    }

    HashString internal_string(const char* str) {
        return internal_string(HashString{str});
    }

public:
    StructStore()
            : own_arena(std::make_unique<CompactArena<1024>>()),
              arena(own_arena->arena), alloc(arena), fields(alloc), slots(alloc) {
        std::cout << "creating a compact arena ..." << std::endl;
    }

    StructStore(const StructStore&) = delete;

    StructStore(StructStore&&) = delete;

    StructStore& operator=(const StructStore&) = delete;

    StructStore& operator=(StructStore&&) = delete;

    size_t allocated_size() const {
        return arena.allocated;
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

    StructStoreField& get_field(HashString name_str) {
        std::cout << "getting field " << name_str.str << std::endl;
        auto it = fields.find(name_str);
        if (it == fields.end()) {
            throw std::runtime_error("field " + std::string(name_str.str) + " does not exist");
        }
        return it->second;
    }

    template<typename T>
    T& get(const char* name) {
        return this->get_hashed<T>(HashString{name});
    }

    template<typename T>
    T& get_hashed(HashString name) {
        auto it = fields.find(name);
        if (it != fields.end()) {
            // field already exists, directly return
            StructStoreField& field = it->second;
            if (field.type != FieldType<T>::value) {
                throw std::runtime_error("field " + std::string(name.str) + " has a different type");
            }
            return field.get<T>();
        }
        T* ptr = ArenaAllocator<T>(arena).allocate(1);
        if constexpr (std::is_base_of<StructStore, T>::value) {
            new(ptr) T(arena);
        } else if constexpr (std::is_same<T, structstore::string>::value) {
            new(ptr) T(alloc);
        } else {
            new(ptr) T();
        }
        T& field = *ptr;
        HashString name_int = internal_string(name);
        fields.emplace(name_int, StructStoreField(field));
        slots.emplace_back(name_int.str);
        return field;
    }

    StructStoreAccess operator[](HashString name) {
        return {*this, name};
    }

    StructStoreAccess operator[](const char* name) {
        return {*this, HashString{name}};
    }
};

template<typename T>
T& StructStoreAccess::get() {
    return store.get_hashed<T>(name);
}

template<>
StructStoreAccess& StructStoreAccess::operator=<const char*>(const char* const& value) {
    get<structstore::string>() = value;
    return *this;
}

std::ostream& operator<<(std::ostream& os, const StructStoreAccess& self) {
    os << self.store.get_field(self.name);
    return os;
}

}

#endif

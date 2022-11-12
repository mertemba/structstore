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

    HashString internal_string(const char* str) {
        size_t len = std::strlen(str);
        char* buf = (char*) arena.allocate(len + 1);
        std::strcpy(buf, str);
        return {buf, const_hash(buf)};
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

    template<typename T>
    T& get(const char* name) {
        HashString name_str = internal_string(name);
        auto it = fields.find(name_str);
        if (it != fields.end()) {
            // field already exists, silently ignore this
            StructStoreField& field = it->second;
            if (field.type != FieldType<T>::value) {
                throw std::runtime_error("field " + std::string(name) + " already exists with a different type");
            }
            return (T&) field;
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
        fields.emplace(name_str, StructStoreField(field));
        slots.emplace_back(name_str.str);
        return field;
    }

    StructStoreField& operator[](HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            throw std::runtime_error("could not find field " + std::string(name.str));
        }
        return it->second;
    }

    StructStoreField& operator[](const char* name) {
        return (*this)[HashString{name}];
    }
};

}

#endif

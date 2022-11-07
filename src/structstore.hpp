#ifndef STRUCTSTORE_HPP
#define STRUCTSTORE_HPP

#include <iostream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "serialization.hpp"
#include "structstore_field.hpp"
#include "structstore_alloc.hpp"
#include "hashstring.hpp"

namespace pybind11 {
template<class type_, class ... options>
struct class_;

struct module_;

struct object;
}

class StructStoreBase {
    friend class StructStoreDyn;

    template<typename T>
    friend pybind11::class_<T> register_pystruct(pybind11::module_&, const char*, bool register_base);

protected:
    Arena arena;
    ArenaAllocator<char> alloc;

private:
    arena_map<HashString, StructStoreField> fields;
    arena_vec<const char*> slots;

protected:
    StructStoreBase(const StructStoreBase&) = delete;

    StructStoreBase(StructStoreBase&&) = delete;

    StructStoreBase& operator=(const StructStoreBase&) = delete;

    StructStoreBase& operator=(StructStoreBase&&) = delete;

    StructStoreBase(size_t bufsize, void* buffer) :
            arena(bufsize, buffer),
            alloc(&arena),
            fields(alloc),
            slots(alloc) {}

    HashString internal_string(const char* str);

    template<typename T>
    void register_field(const char* name, T& field) {
        HashString name_str = internal_string(name);
        fields.emplace(name_str, StructStoreField(field));
        slots.emplace_back(name_str.str);
    }

public:
    StructStoreBase() : StructStoreBase(0, nullptr) {
        throw std::runtime_error("StructStoreBase should not be initialized directly");
    }

    size_t allocated_size() const {
        return arena.allocated;
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStoreBase& self);

    friend YAML::Node to_yaml(const StructStoreBase& self);

    friend pybind11::object to_dict(StructStoreBase& store);
};

template<class Subclass, size_t bufsize = 1024>
class StructStore : public StructStoreBase {
protected:
    StructStore() : StructStoreBase(bufsize, arena_mem.data()) {
        ((Subclass*) this)->list_fields();
    }

private:
    std::array<uint8_t, bufsize> arena_mem = {};
};

#endif

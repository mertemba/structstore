#ifndef STRUCTSTORE_HPP
#define STRUCTSTORE_HPP

#include <iostream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "serialization.hpp"
#include "structstore_alloc.hpp"
#include "hashstring.hpp"

namespace pybind11 {
template<class type_, class ... options>
struct class_;

struct module_;

struct object;
}

namespace structstore {

class StructStoreBase;

template<class, size_t>
class StructStore;

enum class FieldTypeValue : uint8_t {
    INT,
    STRING,
    ARENA_STR,
    BOOL,
    STRUCT,
};

template<typename T, class Enable = void>
struct FieldType;

template<>
struct FieldType<int> {
    static constexpr auto value = FieldTypeValue::INT;
};

template<>
struct FieldType<std::string> {
    static constexpr auto value = FieldTypeValue::STRING;
};

template<>
struct FieldType<arena_str> {
    static constexpr auto value = FieldTypeValue::ARENA_STR;
};

template<>
struct FieldType<bool> {
    static constexpr auto value = FieldTypeValue::BOOL;
};

template<>
struct FieldType<StructStoreBase> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

template<class T>
struct FieldType<T, std::enable_if_t<std::is_base_of_v<StructStoreBase, T>>> {
    static constexpr auto value = FieldTypeValue::STRUCT;
};

class StructStoreField {
private:
    void* data;

public:
    const FieldTypeValue type;

    template<typename T>
    StructStoreField(T& field) : data(&field), type(FieldType<T>::value) {}

    friend std::ostream& operator<<(std::ostream& os, const StructStoreField& self);

    friend YAML::Node to_yaml(const StructStoreField& self);

    template<typename T>
    operator T&() {
        if (type != FieldType<T>::value) {
            throw std::runtime_error("field is not of type " + std::string(typeid(T).name()));
        }
        return *(T*) data;
    }

    template<typename T>
    StructStoreField& operator=(const T& value) {
        (T&) *this = value;
        return *this;
    }
};

class StructStoreBase {
    template<size_t _bufsize>
    friend
    class StructStoreDyn;

    template<typename T>
    friend pybind11::class_ <T> register_pystruct(pybind11::module_&, const char*, pybind11::object* base_cls);

protected:
    bool dynamic = false;
    bool registering_fields = false;
    Arena& arena;
    ArenaAllocator<char> alloc;

private:
    arena_map<HashString, StructStoreField> fields;
    arena_vec<const char*> slots;

protected:
    StructStoreBase(const StructStoreBase&) = delete;

    StructStoreBase(StructStoreBase&&) = delete;

    StructStoreBase& operator=(const StructStoreBase&) = delete;

    StructStoreBase& operator=(StructStoreBase&&) = delete;

    explicit StructStoreBase(Arena& arena)
            : arena(arena), alloc(arena), fields(alloc), slots(alloc) {}

    HashString internal_string(const char* str);

    template<typename T>
    void register_field(const char* name, T& field) {
        if (!registering_fields) {
            return;
        }
        HashString name_str = internal_string(name);
        fields.emplace(name_str, StructStoreField(field));
        slots.emplace_back(name_str.str);
    }

public:
    StructStoreBase() : StructStoreBase(*(Arena*) nullptr) {
        throw std::runtime_error("StructStoreBase should not be initialized directly");
    }

    size_t allocated_size() const {
        return arena.allocated;
    }

    friend std::ostream& operator<<(std::ostream& os, const StructStoreBase& self);

    friend YAML::Node to_yaml(const StructStoreBase& self);

    friend pybind11::object to_dict(StructStoreBase& store);

    bool is_dynamic() const {
        return dynamic;
    }
};

template<class Subclass, size_t _bufsize = 1024>
class StructStore : public StructStoreBase {
public:
    static constexpr size_t bufsize = _bufsize;

protected:
    StructStore() : own_arena(_bufsize, arena_mem.data()), StructStoreBase(own_arena) {
        registering_fields = true;
        ((Subclass*) this)->list_fields();
        registering_fields = false;
    }

    explicit StructStore(Arena& arena) : own_arena(0, nullptr), StructStoreBase(arena) {
        registering_fields = true;
        ((Subclass*) this)->list_fields();
        registering_fields = false;
    }

private:
    std::array<uint8_t, _bufsize> arena_mem = {};
    Arena own_arena;
};

}

#endif

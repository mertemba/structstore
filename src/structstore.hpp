#ifndef STRUCTSTORE_HPP
#define STRUCTSTORE_HPP

#include <iostream>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

#include "hashstring.hpp"
#include "serialization.hpp"
#include "structstore_field.hpp"

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

    StructStoreBase(const StructStoreBase&) = delete;

    StructStoreBase(StructStoreBase&&) = delete;

    StructStoreBase& operator=(const StructStoreBase&) = delete;

    StructStoreBase& operator=(StructStoreBase&&) = delete;

private:
    std::unordered_map<HashString, StructStoreField> fields;
    std::vector<std::string> slots;

protected:
    template<typename T>
    void register_field(const char* name, T& field) {
        fields.emplace(HashString{name}, StructStoreField(field));
        slots.emplace_back(name);
    }

public:
    StructStoreBase() = default;

    friend std::ostream& operator<<(std::ostream& os, const StructStoreBase& self) {
        os << "{";
        for (const auto& [key, value]: self.fields) {
            os << '"' << key.str << "\":" << value << ",";
        }
        os << "}";
        return os;
    }

    friend YAML::Node to_yaml(const StructStoreBase& self) {
        YAML::Node root;
        for (const auto& [key, value]: self.fields) {
            root[key.str] = to_yaml(value);
        }
        return root;
    }

    friend pybind11::object to_dict(StructStoreBase& store);
};

template<class Subclass>
class StructStore : public StructStoreBase {
protected:
    StructStore() {
        ((Subclass*) this)->list_fields();
    }
};

#endif

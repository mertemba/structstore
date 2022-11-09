#include "structstore.hpp"

namespace structstore {

std::ostream& operator<<(std::ostream& os, const StructStoreField& self) {
    ser_text_funcs[self.type](os, self.data);
    return os;
}

YAML::Node to_yaml(const StructStoreField& self) {
    return ser_yaml_funcs[self.type](self.data);
}

HashString StructStoreBase::internal_string(const char* str) {
    size_t len = std::strlen(str);
    char* buf = (char*) arena.allocate(len + 1);
    std::strcpy(buf, str);
    return {buf, const_hash(buf)};
}

std::ostream& operator<<(std::ostream& os, const StructStoreBase& self) {
    os << "{";
    for (const auto& [key, value]: self.fields) {
        os << '"' << key.str << "\":" << value << ",";
    }
    os << "}";
    return os;
}

YAML::Node to_yaml(const StructStoreBase& self) {
    YAML::Node root;
    for (const auto& [key, value]: self.fields) {
        root[key.str] = to_yaml(value);
    }
    return root;
}

}

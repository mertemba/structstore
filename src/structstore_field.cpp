#include "structstore/stst_field.hpp"

using namespace structstore;

std::ostream& structstore::operator<<(std::ostream& os, const StructStoreField& self) {
    serialize_text(os, self.type, self.data);
    return os;
}

YAML::Node structstore::to_yaml(const StructStoreField& self) {
    return serialize_yaml(self.type, self.data);
}

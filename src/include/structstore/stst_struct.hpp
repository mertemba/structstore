#ifndef STST_STRUCT_HPP
#define STST_STRUCT_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_structstore.hpp"

namespace structstore {

class py;

template<typename T>
class Struct : public typing::FieldBase<T> {
    friend class structstore::py;

public:
    MiniMalloc& mm_alloc;
    StructStore store;

    Struct() : Struct(static_alloc) {}

    explicit Struct(MiniMalloc& mm_alloc) : mm_alloc(mm_alloc), store(mm_alloc) {
        store.managed = false;
    }

    Struct(const Struct&) = delete;
    Struct(Struct&&) = delete;
    Struct& operator=(const Struct&) = delete;
    Struct& operator=(Struct&&) = delete;

    void to_text(std::ostream& os) const { os << store; }

    YAML::Node to_yaml() const { return store.to_yaml(); }

    void check(MiniMalloc& mm_alloc) const { store.check(mm_alloc); }

    bool operator==(const Struct& other) const { return store == other.store; }

    bool operator!=(const Struct& other) const { return store != other.store; }
};

} // namespace structstore

#endif

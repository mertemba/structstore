#ifndef STST_STRUCT_HPP
#define STST_STRUCT_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_callstack.hpp"
#include "structstore/stst_fieldmap.hpp"

namespace structstore {

class py;

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
template<typename T>
class Struct : public FieldType<T> {
    friend class structstore::py;

    friend class FieldMap<false>;

private:
    FieldMap<false> field_map;

protected:
    // constructor, assignment, destructor

    Struct() : Struct(static_alloc) {}

    explicit Struct(SharedAlloc& sh_alloc) : field_map(sh_alloc) {}

    Struct(const Struct&) = delete;
    Struct(Struct&&) = delete;
    Struct& operator=(const Struct&) = delete;
    Struct& operator=(Struct&&) = delete;

    ~Struct() { field_map.clear_unmanaged(); }

    // management functions

    template<typename U>
    inline void store_ref(const std::string& name, U& u) {
        field_map.store_ref<U>(name, u, *this);
    }

    void copy_from(const Struct<T>& other) { field_map = other.field_map; }

public:
    // FieldTypeBase utility functions

    inline void to_text(std::ostream& os) const { field_map.to_text(os); }

    inline YAML::Node to_yaml() const { return field_map.to_yaml(); }

    void check(const SharedAlloc* sh_alloc = nullptr) const {
        CallstackEntry entry{"structstore::Struct::check()"};
        field_map.check(sh_alloc, *this);
    }

    inline bool operator==(const Struct& other) const { return field_map == other.field_map; }
};

} // namespace structstore

#endif

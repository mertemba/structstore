#ifndef STST_STRUCT_HPP
#define STST_STRUCT_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_fieldmap.hpp"

namespace structstore {

class py;

template<typename T>
class Struct : public FieldType<T> {
    friend class structstore::py;

    friend class FieldMap<false>;

protected:
    FieldMap<false> field_map;

public:
    // constructor, assignment, destructor

    Struct() : Struct(static_alloc) {}

    explicit Struct(MiniMalloc& mm_alloc) : field_map(mm_alloc) {}

    Struct(const Struct&) = delete;
    Struct(Struct&&) = delete;
    Struct& operator=(const Struct&) = delete;
    Struct& operator=(Struct&&) = delete;

    ~Struct() { field_map.clear_unmanaged(); }

    // FieldTypeBase utility functions

    inline void to_text(std::ostream& os) const { field_map.to_text(os); }

    inline YAML::Node to_yaml() const { return field_map.to_yaml(); }

    void check(const MiniMalloc& mm_alloc, const FieldTypeBase* parent_field) const {
        if (&mm_alloc != &field_map.get_alloc()) {
            throw std::runtime_error("internal error: allocators are not the same");
        }
        if (this->parent_field != parent_field) {
            throw std::runtime_error("invalid parent_field pointer in field of type " +
                                     T::type_info.name);
        }
        field_map.check();
        for (const auto& [key, value]: field_map.get_fields()) {
            try_with_info("in field '" << key.str << "' value: ", value.check(mm_alloc, *this););
        }
    }

    inline bool operator==(const Struct& other) const { return field_map == other.field_map; }
};

} // namespace structstore

#endif

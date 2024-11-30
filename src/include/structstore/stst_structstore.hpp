#ifndef STST_STRUCTSTORE_HPP
#define STST_STRUCTSTORE_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_fieldmap.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

namespace structstore {

class py;

class StructStore : public FieldType<StructStore> {
    friend class py;

public:
    static const TypeInfo& type_info;

protected:
    FieldMap<true> field_map;

public:
    // constructor, assignment, destructor

    explicit StructStore(MiniMalloc& mm_alloc) : field_map(mm_alloc) {}

    StructStore(const StructStore& other) : StructStore{static_alloc} { *this = other; }

    StructStore& operator=(const StructStore& other) {
        field_map.copy_from(other.field_map, this);
        return *this;
    }

    StructStore(StructStore&& other) = delete;
    StructStore& operator=(StructStore&& other) = delete;

    ~StructStore() {
        STST_LOG_DEBUG() << "deconstructing StructStore at " << this;
        clear();
    }

    // FieldTypeBase utility functions

    inline void to_text(std::ostream& os) const { field_map.to_text(os); }

    inline YAML::Node to_yaml() const { return field_map.to_yaml(); }

    void check() const;

    void check(const MiniMalloc& mm_alloc, const FieldTypeBase* parent_field) const;

    inline bool operator==(const StructStore& other) const { return field_map == other.field_map; }

    // query operations

    inline bool empty() const { return field_map.empty(); }

    inline MiniMalloc& get_alloc() { return field_map.get_alloc(); }

    FieldAccess<true> at(HashString name);

    inline FieldAccess<true> at(const char* name) { return at(HashString{name}); }

    // insert operations

    FieldAccess<true> operator[](HashString name);

    inline FieldAccess<true> operator[](const char* name) { return operator[](HashString{name}); }

    template<typename T>
    T& get(const char* name) {
        return (*this)[HashString{name}];
    }

    template<typename T>
    T& get(HashString name) {
        return (*this)[name];
    }

    inline StructStore& substore(const char* name) { return get<StructStore>(name); }

    // remove operations

    inline void clear() { field_map.clear(); }

    inline void remove(HashString name) { field_map.remove(name); }

    inline void remove(const char* name) { remove(HashString{name}); }
};
} // namespace structstore

#endif

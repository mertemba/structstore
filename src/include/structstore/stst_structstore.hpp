#ifndef STST_STRUCTSTORE_HPP
#define STST_STRUCTSTORE_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_fieldmap.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

namespace structstore {

class py;

// todo: when returning a FieldAccess, there should be a read lock on the parent StructStore

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class StructStore : public FieldType<StructStore> {
    friend class ::structstore::typing;
    friend class ::structstore::SharedAlloc;
    friend class ::structstore::StlAllocator<StructStore>;
    friend class ::structstore::StructStoreShared;
    friend class ::structstore::py;

public:
    static const TypeInfo& type_info;

protected:
    FieldMap<true> field_map;

    StructStore(const StructStore& other) : StructStore{static_alloc} { *this = other; }

public:
    // constructor, assignment, destructor

    explicit StructStore(SharedAlloc& sh_alloc) : field_map(sh_alloc) {}

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

    void check(const SharedAlloc* sh_alloc = nullptr) const;

    inline bool operator==(const StructStore& other) const { return field_map == other.field_map; }

    // query operations

    inline bool empty() const { return field_map.empty(); }

    inline SharedAlloc& get_alloc() { return field_map.get_alloc(); }

    FieldAccess<true> at(const std::string& name);

    // insert operations

    FieldAccess<true> operator[](const std::string& name);

    template<typename T>
    T& get(const std::string& name) {
        return (*this)[name];
    }

    StructStore& substore(const std::string& name) { return get<StructStore>(name); }

    // remove operations

    inline void clear() { field_map.clear(); }

    inline void remove(const std::string& name) { field_map.remove(name); }
};

static_assert(std::is_same_v<unwrap_type_t<FieldRef<StructStore>>, StructStore>);
static_assert(std::is_same_v<wrap_type_w<StructStore>, FieldRef<StructStore>>);
} // namespace structstore

#endif

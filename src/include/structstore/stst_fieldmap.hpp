#ifndef STST_FIELDMAP_HPP
#define STST_FIELDMAP_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <stdexcept>
#include <type_traits>

#include <yaml-cpp/yaml.h>

namespace structstore {

// base class for managed and unmanaged FieldMap.
// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class FieldMapBase {
protected:
    OffsetPtr<SharedAlloc> sh_alloc;
    shr_unordered_map<shr_string_ptr, Field, OffsetPtrHasher> fields;
    shr_vector<shr_string_ptr> slots;

    // constructor, assignment, destructor

    explicit FieldMapBase(SharedAlloc& sh_alloc)
        : sh_alloc(&sh_alloc), fields(0, OffsetPtrHasher{this}, StlAllocator<>(sh_alloc)),
          slots(StlAllocator<>(sh_alloc)) {
        STST_LOG_DEBUG() << "constructing FieldMap at " << this << " with alloc at " << &sh_alloc
                         << " (static alloc: " << (&sh_alloc == &static_alloc) << ")";
    }

public:
    // FieldTypeBase utility functions

    void to_text(std::ostream&) const;

    YAML::Node to_yaml() const;

    void check(const SharedAlloc* sh_alloc, const FieldTypeBase& parent_field) const;

    bool equal_slots(const FieldMapBase& other) const;

    bool operator==(const FieldMapBase& other) const;

    inline bool operator!=(const FieldMapBase& other) const { return !(*this == other); }

    // query operations

    inline bool empty() const { return slots.empty(); }

    inline SharedAlloc& get_alloc() { return *sh_alloc; }

    inline const SharedAlloc& get_alloc() const { return *sh_alloc; }

    inline const shr_vector<shr_string_ptr>& get_slots() const { return slots; }

    Field* try_get_field(const std::string& name);

    const Field* try_get_field(const std::string& name) const;

    inline Field& at(const std::string& name) { return fields.at(sh_alloc->strings().get(name)); }

    inline const Field& at(const std::string& name) const {
        return fields.at(sh_alloc->strings().get(name));
    }

    inline Field& at(shr_string_ptr name) { return fields.at(name); }

    inline const Field& at(shr_string_ptr name) const { return fields.at(name); }
};

// managed means the contained fields are allocated and constructed.
// unmanaged means only pointers are stored.
// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
template<bool managed>
class FieldMap : public FieldMapBase {
protected:
    void copy_from_unmanaged(const FieldMap& other);

    void copy_from_managed(const FieldMap& other, const FieldTypeBase* parent_field);

    Field& get_or_insert(const std::string& name);

public:
    // constructor, assignment, destructor

    explicit FieldMap(SharedAlloc& sh_alloc) : FieldMapBase(sh_alloc) {}

    FieldMap(const FieldMap& other) : FieldMap{static_alloc} { *this = other; }

    FieldMap(FieldMap&& other) : FieldMap{static_alloc} { *this = std::move(other); }

    inline FieldMap& operator=(const FieldMap& other) {
        static_assert(!managed, "cannot copy from managed FieldMap; use copy_from() instead");
        copy_from_unmanaged(other);
        return *this;
    }

    FieldMap& operator=(FieldMap&& other) = delete;

    inline void copy_from(const FieldMap& other, const FieldTypeBase* parent_field) {
        static_assert(managed,
                      "cannot copy from unmanaged FieldMap; use assignment operator instead");
        copy_from_managed(other, parent_field);
    }

    ~FieldMap() noexcept(false) {
        STST_LOG_DEBUG() << "deconstructing FieldMap at " << this;
        if (!empty()) {
            throw std::runtime_error("internal error: deconstructor of uncleared FieldMap called!");
        }
    }

    // insert operations

    inline Field& operator[](const std::string& name) {
        static_assert(managed, "potentially creating fields in unmanaged FieldMap is not supported "
                               "(use .at() instead)");
        return get_or_insert(name);
    }

    template<typename T>
    void store_ref(const std::string& name, T& t, const FieldTypeBase& parent_field) {
        static_assert(!managed, "storing ref in managed FieldMap is not supported");
        if constexpr (std::is_base_of_v<Struct<T>, T>) {
            if (t.field_map.sh_alloc != sh_alloc) {
                std::ostringstream str;
                str << "registering Struct field '" << name << "' with a different allocator "
                    << "than this FieldMap, this is probably not what you want";
                throw std::runtime_error(str.str());
            }
        }
        STST_LOG_DEBUG() << "registering unmanaged data at " << &t << "in FieldMap at " << this
                         << " with alloc at " << &sh_alloc
                         << " (static alloc: " << (sh_alloc.get() == &static_alloc) << ")";
        const shr_string* name_int = sh_alloc->strings().internalize(name);
        auto ret = fields.emplace(name_int, Field{&t});
        if (!ret.second) { throw std::runtime_error("field name already exists"); }
        slots.emplace_back(name_int);
        STST_LOG_DEBUG() << "field " << name << " at " << &t;
        if constexpr (std::is_class_v<T> && !std::is_base_of_v<OffsetPtrBase, T>) {
            t.parent_field = &parent_field;
        }
    }

    // remove operations

    void clear() {
        static_assert(managed, "removing fields from unmanaged FieldMap is not supported");
        STST_LOG_DEBUG() << "clearing FieldMap at " << this << "with alloc at " << &sh_alloc;
        if (sh_alloc.get() == &static_alloc) STST_LOG_DEBUG() << "(this is using the static_alloc)";
        for (auto& [key, value]: fields) { value.clear(*sh_alloc); }
        fields.clear();
        slots.clear();
    }

    void clear_unmanaged() {
        static_assert(!managed);
        STST_LOG_DEBUG() << "clearing FieldMap at " << this << "with alloc at " << &sh_alloc;
        if (sh_alloc.get() == &static_alloc) STST_LOG_DEBUG() << "(this is using the static_alloc)";
        for (auto& [key, value]: fields) { value.clear_unmanaged(); }
        fields.clear();
        slots.clear();
    }

    void remove(const std::string& name) {
        static_assert(managed, "removing fields from unmanaged FieldMap is not supported");
        const shr_string* name_ = sh_alloc->strings().get(name);
        Field& field = fields.at(name_);
        field.clear(*sh_alloc);
        fields.erase(name_);
        auto slot_it = std::find(slots.begin(), slots.end(), name_);
        slots.erase(slot_it);
    }
};

} // namespace structstore

#endif

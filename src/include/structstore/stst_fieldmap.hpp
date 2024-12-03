#ifndef STST_FIELDMAP_HPP
#define STST_FIELDMAP_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <stdexcept>
#include <type_traits>

#include <yaml-cpp/yaml.h>

namespace structstore {

// base class for managed and unmanaged FieldMap
class FieldMapBase {
protected:
    MiniMalloc& mm_alloc;
    unordered_map<HashString, Field> fields;
    vector<HashString> slots;

    HashString internalize_string(const HashString& str);

    // constructor, assignment, destructor

    explicit FieldMapBase(MiniMalloc& mm_alloc)
        : mm_alloc(mm_alloc), fields(StlAllocator<>(mm_alloc)), slots(StlAllocator<>(mm_alloc)) {
        STST_LOG_DEBUG() << "constructing FieldMap at " << this << " with alloc at " << &mm_alloc
                         << " (static alloc: " << (&mm_alloc == &static_alloc) << ")";
    }

public:
    // FieldTypeBase utility functions

    void to_text(std::ostream&) const;

    YAML::Node to_yaml() const;

    void check() const;

    bool operator==(const FieldMapBase& other) const;

    inline bool operator!=(const FieldMapBase& other) const { return !(*this == other); }

    // query operations

    inline bool empty() const { return slots.empty(); }

    inline MiniMalloc& get_alloc() { return mm_alloc; }

    inline const MiniMalloc& get_alloc() const { return mm_alloc; }

    inline const unordered_map<HashString, Field>& get_fields() const { return fields; }

    inline const vector<HashString>& get_slots() const { return slots; }

    Field* try_get_field(HashString name);

    const Field* try_get_field(HashString name) const;

    inline Field& at(HashString name) { return fields.at(name); }

    inline const Field& at(HashString name) const { return fields.at(name); }
};

// managed means the contained fields are allocated and constructed.
// unmanaged means only pointers are stored.
template<bool managed>
class FieldMap : public FieldMapBase {
protected:
    void copy_from_unmanaged(const FieldMap& other);

    void copy_from_managed(const FieldMap& other, const FieldTypeBase* parent_field);

    Field& get_or_insert(HashString name);

public:
    // constructor, assignment, destructor

    explicit FieldMap(MiniMalloc& mm_alloc) : FieldMapBase(mm_alloc) {}

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

    inline Field& operator[](HashString name) {
        static_assert(managed, "potentially creating fields in unmanaged FieldMap is not supported "
                               "(use .at() instead)");
        return get_or_insert(name);
    }

    inline Field& operator[](const char* name) { return (*this)[HashString{name}]; }

    template<typename T>
    void store_ref(HashString name, T& t) {
        static_assert(!managed, "storing ref in managed FieldMap is not supported");
        if constexpr (std::is_base_of_v<Struct<T>, T>) {
            if (&t.field_map.mm_alloc != &mm_alloc) {
                std::ostringstream str;
                str << "registering Struct field '" << name.str << "' with a different allocator "
                    << "than this FieldMap, this is probably not what you want";
                throw std::runtime_error(str.str());
            }
        }
        STST_LOG_DEBUG() << "registering unmanaged data at " << &t << "in FieldMap at " << this
                         << " with alloc at " << &mm_alloc
                         << " (static alloc: " << (&mm_alloc == &static_alloc) << ")";
        HashString name_int = internalize_string(name);
        auto ret = fields.emplace(name_int, Field{&t});
        if (!ret.second) { throw std::runtime_error("field name already exists"); }
        slots.emplace_back(name_int);
        STST_LOG_DEBUG() << "field " << name.str << " at " << &t;
#ifndef NDEBUG
        check();
#endif
    }

    template<typename T>
    inline void store_ref(const char* name, T& t) {
        store_ref(HashString{name}, t);
    }

    // remove operations

    void clear() {
        static_assert(managed, "removing fields from unmanaged FieldMap is not supported");
        STST_LOG_DEBUG() << "clearing FieldMap at " << this << "with alloc at " << &mm_alloc;
        if (&mm_alloc == &static_alloc) STST_LOG_DEBUG() << "(this is using the static_alloc)";
#ifndef NDEBUG
        check();
#endif
        for (auto& [key, value]: fields) { value.clear(mm_alloc); }
        fields.clear();
        for (const HashString& str: slots) { mm_alloc.deallocate((void*) str.str); }
        slots.clear();
    }

    void clear_unmanaged() {
        static_assert(!managed);
        STST_LOG_DEBUG() << "clearing FieldMap at " << this << "with alloc at " << &mm_alloc;
        if (&mm_alloc == &static_alloc) STST_LOG_DEBUG() << "(this is using the static_alloc)";
#ifndef NDEBUG
        check();
#endif
        for (auto& [key, value]: fields) { value.clear_unmanaged(); }
        fields.clear();
        for (const HashString& str: slots) { mm_alloc.deallocate((void*) str.str); }
        slots.clear();
    }

    void remove(HashString name) {
        static_assert(managed, "removing fields from unmanaged FieldMap is not supported");
        Field& field = fields.at(name);
        field.clear(mm_alloc);
        fields.erase(name);
        auto slot_it = std::find(slots.begin(), slots.end(), name);
        mm_alloc.deallocate((void*) slot_it->str);
        slots.erase(slot_it);
    }

    void remove(const char* name) { remove(HashString{name}); }
};

} // namespace structstore

#endif

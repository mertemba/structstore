#ifndef STST_STRUCTSTORE_HPP
#define STST_STRUCTSTORE_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_lock.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <type_traits>
#include <unordered_map>

#include <yaml-cpp/yaml.h>

namespace nanobind {
template<class type_, class ... options>
struct class_;

struct module_;

struct object;
}

namespace structstore {

class String;

extern MiniMalloc static_alloc;

class FieldAccess {
    Field& field;
    MiniMalloc& mm_alloc;
    bool managed;

public:
    FieldAccess() = delete;

    FieldAccess(Field& field, MiniMalloc& mm_alloc, bool managed = true)
        : field(field), mm_alloc(mm_alloc), managed(managed) {}

    FieldAccess(const FieldAccess& other) = default;

    FieldAccess& operator=(const FieldAccess& other) = delete;

    template<typename T>
    T& get() {
        static_assert(!std::is_same_v<typename std::remove_cv_t<T>, Field>);
        if (!field.empty()) {
            // field already exists, directly return
            return field.get<T>();
        }
        if (!managed) {
            throw std::runtime_error("cannot create field in StructStore with unmanaged data");
        }
        StlAllocator<T> tmp_alloc{mm_alloc};
        void* ptr = tmp_alloc.allocate(1);
        STST_LOG_DEBUG() << "allocating at " << ptr;
        const auto& type_info = typing::get_type<T>();
        STST_LOG_DEBUG() << "constructing field " << type_info.name << " at " << ptr;
        type_info.constructor_fn(mm_alloc, ptr);
        field.replace_data<T>(ptr, mm_alloc);
        return field.get<T>();
    }

    ::structstore::String& get_str();

    template<typename T>
    operator T&() {
        return get<T>();
    }

    template<typename T>
    FieldAccess& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    Field& get_field() { return field; }

    friend std::ostream& operator<<(std::ostream& os, const FieldAccess& self) {
        return os << self.field;
    }

    [[nodiscard]] uint64_t get_type_hash() const {
        return field.get_type_hash();
    }

    void clear() {
        field.clear(mm_alloc);
    }

    void check() const { check(mm_alloc); }

    void check(MiniMalloc& mm_alloc) const {
        if (&mm_alloc != &this->mm_alloc) {
            throw std::runtime_error("internal error: allocators are not the same");
        }
        if (managed) {
            try_with_info("at FieldAccess: ", field.check(mm_alloc););
        }
    }
};

class AccessView {
    Field field;
    MiniMalloc& mm_alloc;

public:
    template<typename T>
    explicit AccessView(T& data, MiniMalloc& mm_alloc) : field{&data}, mm_alloc{mm_alloc} {}

    template<typename T>
    explicit AccessView(T& data, uint64_t type_hash, MiniMalloc& mm_alloc) : field{&data, type_hash}, mm_alloc{mm_alloc} {}

    FieldAccess operator*() {
        return {field, mm_alloc, false};
    }

    ~AccessView() {
        field.clear_unmanaged();
    }
};

template<>
inline FieldAccess::operator Field&() {
    return get_field();
}

template<>
inline FieldAccess::operator const Field&() {
    return get_field();
}

class StructStoreShared;

class List;

template<typename T>
class Struct;

class py;

class StructStore;

class StructStore : public typing::FieldBase<StructStore> {
    friend class structstore::StructStoreShared;

    friend class structstore::FieldAccess;

    friend class structstore::List;

    template<typename T>
    friend class structstore::Struct;

    friend class structstore::py;

public:
    static const TypeInfo& type_info;

private:
    MiniMalloc& mm_alloc;
    StlAllocator<char> alloc;
    mutable SpinMutex mutex;

    unordered_map<HashString, Field> fields;
    vector<HashString> slots;
    bool managed = true;

    HashString internal_string(const HashString& str) {
        size_t len = std::strlen(str.str);
        char* buf = (char*) mm_alloc.allocate(len + 1);
        std::strcpy(buf, str.str);
        return {buf, str.hash};
    }

public:
    explicit StructStore(MiniMalloc& mm_alloc)
        : mm_alloc(mm_alloc), alloc(mm_alloc), fields(alloc), slots(alloc) {
        STST_LOG_DEBUG() << "constructing StructStore at " << this << " with alloc at " << &mm_alloc
                        << " (static alloc: " << (&mm_alloc == &static_alloc) << ")";
    }

    StructStore(const StructStore& other) : StructStore{static_alloc} {
        STST_LOG_DEBUG() << "copy-constructing StructStore";
        *this = other;
    }

    StructStore(StructStore&& other) : StructStore{static_alloc} {
        STST_LOG_DEBUG() << "move-constructing StructStore";
        *this = std::move(other);
    }

    StructStore& operator=(const StructStore& other) {
        STST_LOG_DEBUG() << "copying StructStore from " << &other << " into " << this;
        if (!managed) {
            if (slots != other.slots) {
                throw std::runtime_error(
                        "copying into non-managed StructStore with different fields");
            }
            for (const HashString& str: slots) {
                fields.at(str).copy_from(mm_alloc, other.fields.at(str));
            }
            return *this;
        }
        // if managed:
        clear();
        for (const HashString& str : other.slots) {
            HashString name_int = internal_string(str);
            slots.emplace_back(name_int);
            Field& field = fields.emplace(name_int, Field{}).first->second;
            field.constr_copy_from(mm_alloc, other.fields.at(str));
        }
        return *this;
    }

    StructStore& operator=(StructStore&& other) {
        STST_LOG_DEBUG() << "moving StructStore from " << &other << " into " << this;
        if (!managed) {
            throw std::runtime_error("moving into non-managed StructStore is not supported");
        }
        if (!other.managed) {
            throw std::runtime_error("moving from non-managed StructStore is not supported");
        }
        if (&mm_alloc == &other.mm_alloc) {
            clear();
            for (const HashString& str : other.slots) {
                slots.emplace_back(str);
                Field& field = fields.emplace(str, Field{}).first->second;
                field.move_from(other.fields.at(str));
            }
            other.slots.clear();
            other.fields.clear();
        } else {
            *this = other;
        }
        return *this;
    }

    ~StructStore() {
        STST_LOG_DEBUG() << "deconstructing StructStore at " << this;
        clear();
    }

    void clear() {
        STST_LOG_DEBUG() << "clearing StructStore at " << this << "with alloc at " << &mm_alloc;
        if (&mm_alloc == &static_alloc) STST_LOG_DEBUG() << "(this is using the static_alloc)";
#ifndef NDEBUG
        check(mm_alloc);
#endif
        for (auto& [key, value]: fields) {
            if (managed) {
                value.clear(mm_alloc);
            } else {
                value.clear_unmanaged();
            }
        }
        fields.clear();
        for (const HashString& str: slots) {
            mm_alloc.deallocate((void*) str.str);
        }
        slots.clear();
    }

    void to_text(std::ostream&) const;

    YAML::Node to_yaml() const;

    void check() const { check(mm_alloc); }

    void check(MiniMalloc& mm_alloc) const;

    friend nanobind::object to_python(const StructStore& store, bool recursive);

    Field* try_get_field(HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            return nullptr;
        }
        return &it->second;
    }

    template<typename T>
    T& get(const char* name) {
        return (*this)[HashString{name}];
    }

    template<typename T>
    T& get(HashString name) {
        return (*this)[name];
    }

    StructStore& substore(const char* name) {
        return get<StructStore>(name);
    }

    FieldAccess operator[](HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            HashString name_int = internal_string(name);
            it = fields.emplace(name_int, Field{}).first;
            slots.emplace_back(name_int);
        }
        return {it->second, mm_alloc, managed};
    }

    FieldAccess operator[](const char* name) {
        return operator[](HashString{name});
    }

    FieldAccess at(HashString name) {
        return {fields.at(name), mm_alloc};
    }

    FieldAccess at(const char* name) {
        return at(HashString{name});
    }

    template<typename T>
    void operator()(HashString name, T& t) {
        if (managed) {
            throw std::runtime_error("cannot register field with existing data in StructStore with only managed data");
        }
        if constexpr (std::is_base_of_v<Struct<T>, T>) {
            if (&t.mm_alloc != &mm_alloc) {
                std::ostringstream str;
                str << "registering Struct field '" << name.str << "' with a different allocator "
                    << "than this StructStore, this is probably not what you want";
                throw std::runtime_error(str.str());
            }
        }
        STST_LOG_DEBUG() << "registering unmanaged data at " << &t << "in StructStore at " << this
                         << " with alloc at " << &mm_alloc
                         << " (static alloc: " << (&mm_alloc == &static_alloc) << ")";
        HashString name_int = internal_string(name);
        auto ret = fields.emplace(name_int, Field{&t});
        if (!ret.second) {
            throw std::runtime_error("field name already exists");
        }
        slots.emplace_back(name_int);
        STST_LOG_DEBUG() << "field " << name.str << " at " << &t;
#ifndef NDEBUG
        check(mm_alloc);
#endif
    }

    template<typename T>
    void operator()(const char* name, T& t) {
        (*this)(HashString{name}, t);
    }

    SpinMutex& get_mutex() {
        return mutex;
    }

    [[nodiscard]] auto write_lock() {
        return ScopedLock(mutex);
    }

    [[nodiscard]] auto read_lock() const {
        return ScopedLock(mutex);
    }

    const vector<HashString>& get_slots() const {
        return slots;
    }

    bool operator==(const StructStore& other) const {
        return slots == other.slots && fields == other.fields;
    }

    bool operator!=(const StructStore& other) const {
        return !(*this == other);
    }
};

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value);

template<>
FieldAccess& FieldAccess::operator= <std::string>(const std::string& value);
}

#endif

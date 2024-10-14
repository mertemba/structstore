#ifndef STST_STRUCTSTORE_HPP
#define STST_STRUCTSTORE_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_hashstring.hpp"
#include "structstore/stst_lock.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <iostream>
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

extern MiniMalloc static_alloc;

class FieldAccess {
    StructStoreField& field;
    MiniMalloc& mm_alloc;
    bool managed;

public:
    FieldAccess() = delete;

    FieldAccess(StructStoreField& field, MiniMalloc& mm_alloc, bool managed = true)
        : field(field), mm_alloc(mm_alloc), managed(managed) {}

    FieldAccess(const FieldAccess& other) = default;

    FieldAccess& operator=(const FieldAccess& other) = delete;

    template<typename T>
    T& get() {
        static_assert(!std::is_same<typename std::remove_cv<T>::type, StructStoreField>::value);
        if (!field.empty()) {
            // field already exists, directly return
            return field.get<T>();
        }
        if (!managed) {
            throw std::runtime_error("cannot create field in StructStore with unmanaged data");
        }
        StlAllocator<T> tmp_alloc{mm_alloc};
        void* ptr = tmp_alloc.allocate(1);
        uint64_t type_hash = typing::get_type_hash<T>();
        const typing::ConstructorFn<>& constructor = typing::get_constructor(type_hash);
        constructor(mm_alloc, ptr);
        field.replace_data<T>(ptr, mm_alloc);
        return field.get<T>();
    }

    ::structstore::string& get_str() {
        return get<::structstore::string>();
    }

    template<typename T>
    operator T&() {
        return get<T>();
    }

    template<typename T>
    FieldAccess& operator=(const T& value) {
        get<T>() = value;
        return *this;
    }

    StructStoreField& get_field() {
        return field;
    }

    friend std::ostream& operator<<(std::ostream& os, const FieldAccess& self) {
        return os << self.field;
    }

    [[nodiscard]] uint64_t get_type_hash() const {
        return field.get_type_hash();
    }

    void clear() {
        field.clear(mm_alloc);
    }

    void check() const {
        try_with_info("at FieldAccess: ", field.check(mm_alloc););
    }
};

class AccessView {
    StructStoreField field;
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
inline FieldAccess::operator StructStoreField&() {
    return get_field();
}

template<>
inline FieldAccess::operator const StructStoreField&() {
    return get_field();
}

class StructStoreShared;

class List;

class Struct;

class py;

class StructStore;

template<>
std::ostream& to_text(std::ostream&, const StructStore&);

template<>
YAML::Node to_yaml(const StructStore&);

class StructStore {
    friend class structstore::StructStoreShared;

    friend class structstore::FieldAccess;

    friend class structstore::List;

    friend class structstore::Struct;

    friend class structstore::py;

    static void register_type();

    friend void typing::register_common_types();

private:
    MiniMalloc& mm_alloc;
    StlAllocator<char> alloc;
    mutable SpinMutex mutex;

    unordered_map<HashString, StructStoreField> fields;
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
            : mm_alloc(mm_alloc), alloc(mm_alloc), fields(alloc), slots(alloc) {}

    StructStore(const StructStore&) = delete;

    StructStore(StructStore&&) = delete;

    StructStore& operator=(const StructStore& other) {
        if (other.fields.empty()) {
            clear();
            return *this;
        }
        throw std::runtime_error("copy assignment of structstore::StructStore is not supported");
    }

    StructStore& operator=(StructStore&&) = delete;

    ~StructStore() {
        clear();
    }

    void clear() {
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

    template<typename T>
    friend std::ostream& structstore::to_text(std::ostream&, const T&);

    template<typename T>
    friend YAML::Node structstore::to_yaml(const T&);

    friend std::ostream& operator<<(std::ostream& os, const StructStore& store) {
        return to_text(os, store);
    }

    friend nanobind::object to_python(const StructStore& store, bool recursive);

    StructStoreField* try_get_field(HashString name) {
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
            it = fields.emplace(name_int, StructStoreField{}).first;
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
        if constexpr (std::is_base_of<Struct, T>::value) {
            if (&t.get_alloc() != &mm_alloc) {
                std::ostringstream str;
                str << "registering Struct field '" << name.str << "' with a different allocator "
                    << "than this StructStore, this is probably not what you want";
                throw std::runtime_error(str.str());
            }
        }
        HashString name_int = internal_string(name);
        auto ret = fields.emplace(name_int, StructStoreField{&t});
        if (!ret.second) {
            throw std::runtime_error("field name already exists");
        }
        slots.emplace_back(name_int);
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

    StructStore& get_store() {
        return *this;
    }

    void check() const {
      for (const HashString &str : slots) {
        try_with_info("in slot '" << str.str << "' name: ",
                      mm_alloc.assert_owned(str.str););
      }
      for (const auto &[key, value] : fields) {
        try_with_info("in field '" << key.str << "' name: ",
                      mm_alloc.assert_owned(key.str););
        if (managed) {
            try_with_info("in field '" << key.str << "' value: ",
                          value.check(mm_alloc););
        }
      }
    }
};

template<>
FieldAccess& FieldAccess::operator=<const char*>(const char* const& value);

template<>
FieldAccess& FieldAccess::operator=<std::string>(const std::string& value);

class py;

class Struct;

static const StructStore& get_store(const Struct&);

static StructStore& get_store(Struct&);

class Struct {
    friend class structstore::py;

    friend class structstore::StructStore;

    friend const StructStore& structstore::get_store(const Struct&);

    friend StructStore& structstore::get_store(Struct&);

private:
    MiniMalloc& mm_alloc;

protected:
    StructStore store;

    Struct() : Struct(static_alloc) {}

    explicit Struct(MiniMalloc& mm_alloc) : mm_alloc(mm_alloc), store(mm_alloc) {
        store.managed = false;
    }

    Struct(const Struct&) = delete;
    Struct(Struct&&) = delete;
    Struct& operator=(const Struct&) = delete;
    Struct& operator=(Struct&&) = delete;

    StructStore& get_store() {
        return store;
    }

    MiniMalloc& get_alloc() {
        return mm_alloc;
    }

    StlAllocator<char> get_stl_alloc() {
        return StlAllocator<char>{mm_alloc};
    }

    template<typename T>
    friend std::ostream& structstore::to_text(std::ostream&, const T&);

    template<typename T>
    friend YAML::Node structstore::to_yaml(const T&);

    friend std::ostream& operator<<(std::ostream&, const Struct&);
};

template<>
inline std::ostream& to_text(std::ostream& os, const Struct& struct_) {
    return to_text(os, struct_.store);
}

template<>
inline YAML::Node to_yaml(const Struct& struct_) {
    return to_yaml(struct_.store);
}

inline std::ostream& operator<<(std::ostream& os, const Struct& struct_) {
    return to_text(os, struct_);
}

static inline const StructStore& get_store(const Struct& str) {
    return str.store;
}

static inline StructStore& get_store(Struct& str) {
    return str.store;
}

}

#endif

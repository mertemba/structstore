#ifndef STRUCTSTORE_DYN_HPP
#define STRUCTSTORE_DYN_HPP

#include "structstore.hpp"

template<size_t _bufsize = 1024>
struct StructStoreDyn : StructStore<StructStoreDyn<_bufsize>, _bufsize> {
private:
    using Base = StructStore<StructStoreDyn<_bufsize>, _bufsize>;

public:
    StructStoreDyn() : Base() {
        Base::dynamic = true;
    }

    explicit StructStoreDyn(Arena& arena) : Base(arena) {
        Base::dynamic = true;
    }

    template<typename T>
    T& add_field(const char* name) {
        HashString name_str = Base::internal_string(name);
        auto it = Base::fields.find(name_str);
        if (it != Base::fields.end()) {
            // field already exists, silently ignore this
            StructStoreField& field = it->second;
            if (field.type != FieldType<T>::value) {
                throw std::runtime_error("field " + std::string(name) + " already exists with a different type");
            }
            return (T&) field;
        }
        T* ptr = ArenaAllocator<T>(StructStoreBase::arena).allocate(1);
        if constexpr (std::is_base_of<StructStoreBase, T>::value) {
            static_assert(T::bufsize == 0);
            new(ptr) T(StructStoreBase::arena);
        } else {
            new(ptr) T();
        }
        T& field = *ptr;
        Base::fields.emplace(name_str, StructStoreField(field));
        Base::slots.emplace_back(name_str.str);
        return field;
    }

    StructStoreField& operator[](HashString name) {
        auto it = Base::fields.find(name);
        if (it == Base::fields.end()) {
            throw std::runtime_error("could not find field " + std::string(name.str));
        }
        return it->second;
    }

    StructStoreField& operator[](const char* name) {
        return (*this)[HashString{name}];
    }

public:
    void list_fields() {}
};

#endif

#ifndef STRUCTSTORE_DYN_HPP
#define STRUCTSTORE_DYN_HPP

#include "structstore.hpp"

struct StructStoreDyn : StructStore<StructStoreDyn> {
    friend class StructStore<StructStoreDyn>;

    template<typename T>
    T& add_field(const char* name) {
        HashString name_str = internal_string(name);
        T* ptr = ArenaAllocator<T>(arena).allocate(1);
        new(ptr) T();
        T& field = *ptr;
        fields.emplace(name_str, StructStoreField(field));
        slots.emplace_back(name_str.str);
        return field;
    }

    StructStoreField& operator[](HashString name) {
        auto it = fields.find(name);
        if (it == fields.end()) {
            throw std::runtime_error("could not find field " + std::string(name.str));
        }
        return it->second;
    }

    StructStoreField& operator[](const char* name) {
        return (*this)[HashString{name}];
    }

private:
    void list_fields() {}
};

#endif

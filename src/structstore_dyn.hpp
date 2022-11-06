#ifndef STRUCTSTORE_DYN_HPP
#define STRUCTSTORE_DYN_HPP

#include "structstore.hpp"

struct StructStoreDyn : StructStore<StructStoreDyn> {
    friend class StructStore<StructStoreDyn>;

    template<typename T>
    T& add_field(const char* name) {
        // TODO: use ManagedHashString instead?
        const char* name_ = strdup(name);
        std::shared_ptr<T> ptr{new T()};
        T& field = *ptr;
        fields_data.emplace_back(std::move(ptr));
        fields.emplace(HashString{name_}, StructStoreField(field));
        slots.emplace_back(name_);
        return field;
    }

    StructStoreField& operator[](const char* name) {
        HashString name_str{name};
        auto it = fields.find(name_str);
        if (it == fields.end()) {
            throw std::runtime_error("could not find field " + std::string(name));
        }
        return it->second;
    }

private:
    void list_fields() {}

    std::vector<std::shared_ptr<void>> fields_data;
};

#endif

#ifndef STRUCTSTORE_CONTAINERS_HPP
#define STRUCTSTORE_CONTAINERS_HPP

#include "structstore.hpp"

namespace structstore {

class List {
    MiniMalloc& mm_alloc;
    structstore::vector<StructStoreField> data;
    mutable SpinMutex mutex;

public:
    class Iterator {
        const List& list;
        size_t idx;

    public:
        Iterator(const List& list, size_t idx) : list(list), idx(idx) {}

        bool operator==(Iterator& other) const {
            return &list == &other.list && idx == other.idx;
        }

        bool operator!=(Iterator& other) const {
            return !(*this == other);
        }

        Iterator& operator++() {
            ++idx;
            return *this;
        }

        StructStoreField& operator*() {
            return ((List&) list).data[idx];
        }
    };

    List() : mm_alloc(*(MiniMalloc*) nullptr), data(StlAllocator<StructStoreField>(mm_alloc)) {
        throw std::runtime_error("List should not be constructed without an allocator");
    }

    explicit List(MiniMalloc& mm_alloc) : mm_alloc(mm_alloc), data(StlAllocator<StructStoreField>(mm_alloc)) {}

    List(const List&) = delete;

    List(List&&) = delete;

    List& operator=(const List&) = delete;

    List& operator=(List&&) = delete;

    FieldAccess push_back() {
        return {data.emplace_back(), mm_alloc};
    }

    template<typename T>
    void push_back(const T& value) {
        data.emplace_back(StlAllocator<T>(mm_alloc).allocate(1)) = value;
    }

    FieldAccess operator[](size_t idx) {
        return {data[idx], mm_alloc};
    }

    Iterator begin() const {
        return {*this, 0};
    }

    Iterator end() const {
        return {*this, data.size()};
    }

    friend std::ostream& operator<<(std::ostream& os, const List& self) {
        os << "[";
        for (const StructStoreField& field: self.data) {
            os << field << ",";
        }
        return os << "]";
    }

    void clear() {
        data.clear();
    }

    [[nodiscard]] auto write_lock() {
        return ScopedLock(mutex);
    }

    [[nodiscard]] auto read_lock() const {
        return ScopedLock(mutex);
    }
};

template<>
void List::push_back<const char*>(const char* const& value) {
    push_back().get<structstore::string>() = value;
}

YAML::Node to_yaml(const List& list) {
    auto node = YAML::Node();
    for (const StructStoreField& field: list) {
        node.push_back(to_yaml(field));
    }
    return node;
}

}

#endif

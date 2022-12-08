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

class Matrix {
    MiniMalloc& mm_alloc;
    size_t _rows, _cols;
    double* _data;

public:
    Matrix(MiniMalloc& mm_alloc) : Matrix(0, 0, mm_alloc) {}

    Matrix(size_t rows, size_t cols, MiniMalloc& mm_alloc) : mm_alloc(mm_alloc), _rows(rows), _cols(cols) {
        if (rows == 0 || cols == 0) {
            _data = nullptr;
        } else {
            _data = (double*) mm_alloc.allocate(sizeof(double) * rows * cols);
        }
    }

    ~Matrix() {
        if (_data != nullptr) {
            mm_alloc.deallocate(_data);
        }
    }

    Matrix(Matrix&&) = delete;

    Matrix(const Matrix& other) : Matrix(0, 0, other.mm_alloc) {
        *this = other;
    }

    Matrix& operator=(Matrix&& other) {
        if (&mm_alloc == &other.mm_alloc) {
            _data = other._data;
        } else {
            const Matrix& other_ = other;
            *this = other_;
        }
        other._data = nullptr;
        return *this;
    }

    Matrix& operator=(const Matrix& other) {
        if (_data != nullptr) {
            mm_alloc.deallocate(_data);
        }
        _rows = other._rows;
        _cols = other._cols;
        _data = (double*) mm_alloc.allocate(sizeof(double) * _rows * _cols);
        std::memcpy(_data, other._data, sizeof(double) * _rows * _cols);
        return *this;
    }

    double* data() { return _data; }

    size_t rows() const { return _rows; }

    size_t cols() const { return _cols; }

    void from(size_t rows, size_t cols, double* data) {
        if (_data != nullptr) {
            mm_alloc.deallocate(_data);
        }
        _rows = rows;
        _cols = cols;
        _data = (double*) mm_alloc.allocate(sizeof(double) * _rows * _cols);
        std::memcpy(_data, data, sizeof(double) * _rows * _cols);
    }
};

}

#endif

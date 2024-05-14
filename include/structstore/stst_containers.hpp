#ifndef STST_CONTAINERS_HPP
#define STST_CONTAINERS_HPP

#include "structstore/stst_structstore.hpp"
#include "structstore/stst_field.hpp"

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

    ~List() {
        clear();
    }

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

    FieldAccess insert(size_t index) {
        return {*data.emplace(data.begin() + index), mm_alloc};
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

    friend std::ostream& operator<<(std::ostream& os, const List& self);

    SpinMutex& get_mutex() {
        return mutex;
    }

    size_t size() {
        return data.size();
    }

    void erase(size_t index) {
        data[index].clear(mm_alloc);
        data.erase(data.begin() + index);
    }

    void clear() {
        for (StructStoreField& field: data) {
            field.clear(mm_alloc);
        }
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
void List::push_back<const char*>(const char* const& value);

class Matrix {
    MiniMalloc& mm_alloc;
    size_t _rows, _cols;
    double* _data;
    bool _is_vector = false;

public:
    Matrix(MiniMalloc& mm_alloc) : Matrix(0, 0, mm_alloc) {}

    Matrix(size_t rows, size_t cols, MiniMalloc& mm_alloc, bool is_vector = false)
            : mm_alloc(mm_alloc), _rows(rows), _cols(cols), _is_vector(is_vector) {
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
        from(other._rows, other._cols, other._data, other._is_vector);
        return *this;
    }

    double* data() { return _data; }

    size_t rows() const { return _rows; }

    size_t cols() const { return _cols; }

    bool is_vector() const { return _is_vector; }

    void from(size_t rows, size_t cols, const double* data, bool is_vector) {
        if (data == _data) {
            if (rows != _rows || cols != _cols) {
                throw std::runtime_error("setting matrix data to same pointer but different size");
            }
            _is_vector = is_vector;
            return;
        }
        if (_data == nullptr || rows != _rows || cols != _cols) {
            if (_data != nullptr) {
                mm_alloc.deallocate(_data);
            }
            _rows = rows;
            _cols = cols;
            _is_vector = is_vector;
            _data = (double*) mm_alloc.allocate(sizeof(double) * _rows * _cols);
        }
        std::memcpy(_data, data, sizeof(double) * _rows * _cols);
    }

    friend std::ostream& operator<<(std::ostream& os, const Matrix& self);
};

}

#endif

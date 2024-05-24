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
public:
    static constexpr int MAX_DIMS = 8;

protected:
    MiniMalloc& mm_alloc;
    size_t _ndim;
    size_t _shape[MAX_DIMS] = {};
    double* _data;

public:
    Matrix(MiniMalloc& mm_alloc) : Matrix(0, 0, mm_alloc) {}

    Matrix(size_t ndim, const size_t* shape, MiniMalloc& mm_alloc)
            : mm_alloc(mm_alloc), _ndim(ndim) {
        if (ndim == 0) {
            _data = nullptr;
        } else {
            from(ndim, shape, nullptr);
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
            std::swap(_ndim, other._ndim);
            std::swap(_shape, other._shape);
            std::swap(_data, other._data);
        } else {
            const Matrix& other_ = other;
            *this = other_;
        }
        return *this;
    }

    Matrix& operator=(const Matrix& other) {
        from(other._ndim, other._shape, other._data);
        return *this;
    }

    size_t ndim() const { return _ndim; }

    const size_t* shape() const { return _shape; }

    double* data() { return _data; }

    void from(size_t ndim, const size_t* shape, const double* data) {
        if (data == _data) {
            if (ndim != _ndim) {
                throw std::runtime_error("setting matrix data to same pointer but different size");
            }
            for (size_t i = 0; i < ndim; ++i) {
                if (shape[i] != _shape[i]) {
                    throw std::runtime_error("setting matrix data to same pointer but different size");
                }
            }
            return;
        }
        if (_data != nullptr) {
            mm_alloc.deallocate(_data);
            _data = nullptr;
        }
        _ndim = ndim;
        size_t size = sizeof(double);
        for (size_t i = 0; i < ndim; ++i) {
            if ((ssize_t) shape[i] < 0) {
                throw std::runtime_error("initializing matrix with invalid shape");
            }
            _shape[i] = shape[i];
            size *= shape[i];
        }
        if (size > 0) {
            _data = (double*) mm_alloc.allocate(size);
            if (data != nullptr) {
                std::memcpy(_data, data, size);
            }
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Matrix& self);
};

}

#endif

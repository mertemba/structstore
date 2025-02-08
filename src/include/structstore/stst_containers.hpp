#ifndef STST_CONTAINERS_HPP
#define STST_CONTAINERS_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_field.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

namespace structstore {

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class String : public FieldType<String>, public shr_string {
public:
    static const TypeInfo& type_info;

    String(const StlAllocator<String>& alloc) : shr_string(alloc) {}

    void to_text(std::ostream& os) const { os << static_cast<const shr_string&>(*this); }

    YAML::Node to_yaml() const { return YAML::Node(c_str()); }

    void check(const SharedAlloc* sh_alloc = nullptr) const;

    String& operator=(const std::string& value);
};

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class List : public FieldType<List> {
    OffsetPtr<SharedAlloc, int64_t> sh_alloc;
    shr_vector<Field> data;

public:
    static const TypeInfo& type_info;

    class Iterator {
        ScopedFieldLock<false> scoped_lock;
        List& list;
        size_t index;

    public:
        Iterator(List& list, size_t index, ScopedFieldLock<false>&& scoped_lock)
            : scoped_lock(std::move(scoped_lock)), list(list), index(index) {}

        Iterator(Iterator&& other) = default;
        ~Iterator() = default;

        Iterator(const Iterator& other) = delete;
        Iterator& operator=(const Iterator& other) = delete;
        Iterator& operator=(Iterator&& other) = delete;

        bool operator==(const Iterator& other) const {
            return &list == &other.list && index == other.index;
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }

        Iterator& operator++() {
            ++index;
            return *this;
        }

        Field& operator*() { return ((List&) list).data.at(index); }
    };

    explicit List(SharedAlloc& sh_alloc)
        : sh_alloc(&sh_alloc), data(StlAllocator<Field>(sh_alloc)) {
        STST_LOG_DEBUG() << "constructing List at " << this;
    }

    ~List() {
        STST_LOG_DEBUG() << "destructing List at " << this;
        clear();
    }

    List(const List& other) = delete;
    List(List&&) = delete;

    List& operator=(const List& other) {
        if (other.data.empty()) {
            clear();
            return *this;
        }
        throw std::runtime_error("copy assignment of structstore::List is not supported");
    }

    List& operator=(List&&) = delete;

    FieldAccess<true> push_back() {
        STST_LOG_DEBUG() << "this: " << this << ", cur size: " << data.size();
        return FieldAccess<true>{data.emplace_back(), *sh_alloc, this};
    }

    template<typename T>
    inline void push_back(const T& value) {
        push_back() = value;
    }

    FieldAccess<true> insert(size_t index) {
        if (index > data.size()) {
            throw std::out_of_range("index out of bounds: " + std::to_string(index));
        }
        return FieldAccess<true>{*data.emplace(data.begin() + index), *sh_alloc, this};
    }

    FieldAccess<true> operator[](size_t index) {
        if (index >= data.size()) {
            throw std::out_of_range("index out of bounds: " + std::to_string(index));
        }
        return FieldAccess<true>{data.at(index), *sh_alloc, this};
    }

    FieldAccess<true> at(size_t index) {
        return FieldAccess<true>{data.at(index), *sh_alloc, this};
    }

    Iterator begin() const { return {(List&) *this, 0, read_lock()}; }

    Iterator end() const { return {(List&) *this, data.size(), read_lock()}; }

    size_t size() {
        return data.size();
    }

    void erase(size_t index) {
        if (index >= data.size()) {
            throw std::out_of_range("index out of bounds: " + std::to_string(index));
        }
        at(index).clear();
        data.erase(data.begin() + index);
    }

    void clear() {
        for (Field& field: data) { FieldAccess<true>{field, *sh_alloc, this}.clear(); }
        data.clear();
    }

    void to_text(std::ostream&) const;

    YAML::Node to_yaml() const;

    void check(const SharedAlloc* sh_alloc = nullptr) const;

    bool operator==(const List& other) const;
};

// instances of this class reside in shared memory, thus no raw pointers
// or references should be used; use structstore::OffsetPtr<T> instead.
class Matrix : public FieldType<Matrix> {
public:
    static constexpr int MAX_DIMS = 8;

    static const TypeInfo& type_info;

protected:
    OffsetPtr<SharedAlloc> sh_alloc;
    size_t _ndim;
    size_t _shape[MAX_DIMS] = {};
    OffsetPtr<double> _data;

public:
    Matrix(SharedAlloc& sh_alloc) : Matrix(0, 0, sh_alloc) {}

    Matrix(size_t ndim, const size_t* shape, SharedAlloc& sh_alloc)
        : sh_alloc(&sh_alloc), _ndim(ndim) {
        if (ndim == 0) {
            _data = nullptr;
        } else {
            from(ndim, shape, nullptr);
        }
    }

    ~Matrix() {
        if (_data) { sh_alloc->deallocate(_data.get()); }
    }

    Matrix(Matrix&&) = delete;
    Matrix(const Matrix&) = delete;

    Matrix& operator=(Matrix&& other) {
        if (&sh_alloc != &other.sh_alloc) {
            throw std::runtime_error("move assignment of structstore::Matrix between different StructStores is not supported");
        }
        std::swap(_ndim, other._ndim);
        std::swap(_shape, other._shape);
        std::swap(_data, other._data);
        return *this;
    }

    Matrix& operator=(const Matrix& other) {
        from(other._ndim, other._shape, other._data.get());
        return *this;
    }

    size_t ndim() const { return _ndim; }

    const size_t* shape() const { return _shape; }

    double* data() { return _data.get(); }

    void from(size_t ndim, const size_t* shape, const double* data) {
        if (data == _data.get()) {
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
        if (_data) {
            sh_alloc->deallocate(_data.get());
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
            _data = (double*) sh_alloc->allocate(size);
            if (data != nullptr) { std::memcpy(_data.get(), data, size); }
        }
    }

    void to_text(std::ostream&) const;

    YAML::Node to_yaml() const;

    void check(const SharedAlloc* sh_alloc = nullptr) const;

    bool operator==(const Matrix& other) const;
};
}

#endif

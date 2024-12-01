#ifndef STST_LOCK_HPP
#define STST_LOCK_HPP

#include <atomic>

namespace structstore {

class SpinMutex {
    template<bool write>
    friend class ScopedLock;

    friend class FieldTypeBase;

    // randomly assigned pseudo thread id
    static thread_local uint32_t tid;

    // >0 is read-locked, 0 is unlocked, <0 is write-locked
    std::atomic_int32_t level{0};
    // thread id currently holding the write lock, or zero
    uint32_t write_lock_tid{0};

    SpinMutex(SpinMutex&&) = delete;

    SpinMutex(const SpinMutex&) = delete;

    SpinMutex& operator=(SpinMutex&&) = delete;

    SpinMutex& operator=(const SpinMutex&) = delete;

    void read_lock();
    void read_unlock();

    void write_lock();
    void write_unlock();

public:
    SpinMutex() = default;
};

template<bool write>
class ScopedLock {
    SpinMutex* mutex = nullptr;

    ScopedLock() : mutex{nullptr} {}

public:
    explicit ScopedLock(SpinMutex& mutex);

    ScopedLock(ScopedLock&& other) noexcept : ScopedLock() { *this = std::move(other); }

    ScopedLock& operator=(ScopedLock&& other) noexcept {
        std::swap(mutex, other.mutex);
        return *this;
    }

    ScopedLock(const ScopedLock&) = delete;
    ScopedLock& operator=(const ScopedLock&) = delete;

    ~ScopedLock() { unlock(); }

    void unlock();
};

template<>
ScopedLock<false>::ScopedLock(SpinMutex& mutex);
template<>
ScopedLock<true>::ScopedLock(SpinMutex& mutex);

class FieldTypeBase;

template<bool write>
class ScopedFieldLock {
    const FieldTypeBase* field = nullptr;

    ScopedFieldLock() : field{nullptr} {}

public:
    explicit ScopedFieldLock(const FieldTypeBase& field);

    ScopedFieldLock(ScopedFieldLock&& other) noexcept : ScopedFieldLock() {
        *this = std::move(other);
    }

    ScopedFieldLock& operator=(ScopedFieldLock&& other) noexcept {
        std::swap(field, other.field);
        return *this;
    }

    ScopedFieldLock(const ScopedFieldLock&) = delete;
    ScopedFieldLock& operator=(const ScopedFieldLock&) = delete;

    ~ScopedFieldLock() { unlock(); }

    void unlock();
};

template<>
ScopedFieldLock<false>::ScopedFieldLock(const FieldTypeBase& field);
template<>
ScopedFieldLock<true>::ScopedFieldLock(const FieldTypeBase& field);
}

#endif

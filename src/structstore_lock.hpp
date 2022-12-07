#ifndef STRUCTSTORE_LOCK_HPP
#define STRUCTSTORE_LOCK_HPP

#include <atomic>

namespace structstore {

class SpinMutex {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    SpinMutex() = default;

    SpinMutex(SpinMutex&&) = delete;

    SpinMutex(const SpinMutex&) = delete;

    SpinMutex& operator=(SpinMutex&&) = delete;

    SpinMutex& operator=(const SpinMutex&) = delete;

    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
#if defined(__cpp_lib_atomic_flag_test)
            while (flag.test(std::memory_order_relaxed));
#endif
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};

class ScopedLock {
    SpinMutex* mutex;

public:
    explicit ScopedLock(SpinMutex& mutex) : mutex(&mutex) {
        mutex.lock();
    }

    ~ScopedLock() {
        if (mutex) {
            mutex->unlock();
        }
    }

    ScopedLock(ScopedLock&& other) noexcept {
        mutex = other.mutex;
        other.mutex = nullptr;
    }

    ScopedLock& operator=(ScopedLock&& other) noexcept {
        mutex = other.mutex;
        other.mutex = nullptr;
        return *this;
    }

    ScopedLock(const ScopedLock&) = delete;

    ScopedLock& operator=(const ScopedLock&) = delete;
};

}

#endif

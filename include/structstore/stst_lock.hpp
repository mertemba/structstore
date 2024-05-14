#ifndef STST_LOCK_HPP
#define STST_LOCK_HPP

#include <unistd.h>

#include <atomic>

namespace structstore {

class SpinMutex {
    std::atomic_int flag{0};
    int lock_level = 0;

    // too many keywords
    inline static thread_local const int tid = gettid();

public:

    SpinMutex() = default;

    SpinMutex(SpinMutex&&) = delete;

    SpinMutex(const SpinMutex&) = delete;

    SpinMutex& operator=(SpinMutex&&) = delete;

    SpinMutex& operator=(const SpinMutex&) = delete;

    void lock() {
        int v = flag.load(std::memory_order_relaxed);
        if (v == tid) {
            lock_level++;
            return;
        }
        v = 0;
        while (!flag.compare_exchange_strong(v, tid, std::memory_order_acquire)) {
            while ((v = flag.load(std::memory_order_relaxed)) != 0) { }
        }
        lock_level++;
    }

    void unlock() {
        if ((lock_level -= 1) == 0) {
            flag.store(0, std::memory_order_release);
        }
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

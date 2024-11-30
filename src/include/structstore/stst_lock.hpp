#ifndef STST_LOCK_HPP
#define STST_LOCK_HPP

#include "structstore/stst_utils.hpp"

#include <atomic>
#include <cstdint>
#include <unistd.h>

namespace structstore {

class SpinMutex {
    std::atomic_int32_t flag{0};
    int32_t lock_level = 0;

    // too many keywords
    inline static thread_local const int32_t tid = gettid();

public:

    SpinMutex() = default;

    SpinMutex(SpinMutex&&) = delete;

    SpinMutex(const SpinMutex&) = delete;

    SpinMutex& operator=(SpinMutex&&) = delete;

    SpinMutex& operator=(const SpinMutex&) = delete;

    void lock() {
        STST_LOG_DEBUG() << "locking " << this;
        int32_t v = flag.load(std::memory_order_relaxed);
        if (v == tid) {
            ++lock_level;
            STST_LOG_DEBUG() << "already locked " << this << " at level " << lock_level;
            return;
        }
        v = 0;
        while (!flag.compare_exchange_strong(v, tid, std::memory_order_acquire)) {
            while ((v = flag.load(std::memory_order_relaxed)) != 0) { }
        }
        ++lock_level;
        STST_LOG_DEBUG() << "got lock at " << this << " at level " << lock_level;
    }

    void unlock() {
        if ((--lock_level) == 0) {
            STST_LOG_DEBUG() << "completely unlocked " << this;
            flag.store(0, std::memory_order_release);
        }
        STST_LOG_DEBUG() << "unlocking " << this << " to level " << lock_level;
    }
};

class ScopedLock {
    SpinMutex* mutex = nullptr;

    ScopedLock() : mutex{nullptr} {}

public:
    explicit ScopedLock(SpinMutex& mutex) : mutex(&mutex) {
        mutex.lock();
    }

    void unlock() {
        if (mutex) {
            mutex->unlock();
            mutex = nullptr;
        }
    }

    ~ScopedLock() {
        unlock();
    }

    ScopedLock(ScopedLock&& other) noexcept: ScopedLock() {
        *this = std::move(other);
    }

    ScopedLock& operator=(ScopedLock&& other) noexcept {
        std::swap(mutex, other.mutex);
        return *this;
    }

    ScopedLock(const ScopedLock&) = delete;

    ScopedLock& operator=(const ScopedLock&) = delete;
};

}

#endif

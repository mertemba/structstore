#include "structstore/stst_lock.hpp"
#include "structstore/stst_typing.hpp"
#include "structstore/stst_utils.hpp"

#include <chrono>
#include <random>
#include <stdexcept>
#include <thread>

using namespace structstore;

thread_local uint32_t SpinMutex::tid = std::random_device{}();

#define TIMEOUT_CHECK_INIT                                                                         \
    int cnt = 0;                                                                                   \
    double start_time = 0.0;

#define TIMEOUT_CHECK(lock_type)                                                                   \
    do {                                                                                           \
        ++cnt;                                                                                     \
        if (cnt > 1'000'000) {                                                                     \
            std::this_thread::yield();                                                             \
            cnt = 0;                                                                               \
            double now = std::chrono::duration<double>(                                            \
                                 std::chrono::steady_clock::now().time_since_epoch())              \
                                 .count();                                                         \
            if (start_time == 0.0) {                                                               \
                start_time = now;                                                                  \
            } else {                                                                               \
                if (now - start_time > 0.1) {                                                      \
                    throw std::runtime_error("timeout while getting " lock_type);                  \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    } while (0)

void SpinMutex::read_lock() {
    STST_LOG_DEBUG() << "read locking " << this;
    if (write_lock_tid == tid) {
        throw std::runtime_error("trying to acquire read lock while current thread has write lock");
    }
    int32_t v;
    TIMEOUT_CHECK_INIT
    do {
        while ((v = level.load(std::memory_order_relaxed)) < 0) { TIMEOUT_CHECK("read lock"); }
    } while (!level.compare_exchange_weak(v, v + 1, std::memory_order_acquire));
    STST_LOG_DEBUG() << "read locked at " << this << " at level " << v + 1;
}

void SpinMutex::read_unlock() {
    STST_LOG_DEBUG() << "read unlocking " << this;
    int32_t v = level.load(std::memory_order_relaxed);
    while (!level.compare_exchange_weak(v, v - 1, std::memory_order_acquire));
    STST_LOG_DEBUG() << "read unlocked at " << this << " at level " << v - 1;
}

void SpinMutex::write_lock() {
    STST_LOG_DEBUG() << "write locking " << this;
    int32_t v;
    if (write_lock_tid == tid) {
        v = level.load(std::memory_order_relaxed);
        level.store(v - 1, std::memory_order_relaxed);
        return;
    }
    TIMEOUT_CHECK_INIT
    do {
        while ((v = level.load(std::memory_order_relaxed)) != 0) { TIMEOUT_CHECK("write lock"); }
    } while (!level.compare_exchange_weak(v, -1, std::memory_order_acquire));
    if (write_lock_tid != 0) { throw std::runtime_error("internal error: write_lock_tid != 0"); }
    write_lock_tid = tid;
    STST_LOG_DEBUG() << "write locked at " << this;
}

void SpinMutex::write_unlock() {
    int32_t v = level.load(std::memory_order_relaxed);
    level.store(v + 1, std::memory_order_release);
    if (v + 1 == 0) { write_lock_tid = 0; }
    STST_LOG_DEBUG() << "write unlocked " << this;
}

template<>
ScopedLock<false>::ScopedLock(SpinMutex& mutex) : mutex(&mutex) {
    mutex.read_lock();
}

template<>
ScopedLock<true>::ScopedLock(SpinMutex& mutex) : mutex(&mutex) {
    mutex.write_lock();
}

template<>
void ScopedLock<false>::unlock() {
    if (mutex) {
        mutex->read_unlock();
        mutex = nullptr;
    }
}

template<>
void ScopedLock<true>::unlock() {
    if (mutex) {
        mutex->write_unlock();
        mutex = nullptr;
    }
}

template<>
ScopedFieldLock<false>::ScopedFieldLock(const FieldTypeBase& field) : field(&field) {
    field.read_lock_();
}

template<>
ScopedFieldLock<true>::ScopedFieldLock(const FieldTypeBase& field) : field(&field) {
    field.write_lock_();
}

template<>
void ScopedFieldLock<false>::unlock() {
    if (field) {
        field->read_unlock_();
        field = nullptr;
    }
}

template<>
void ScopedFieldLock<true>::unlock() {
    if (field) {
        field->write_unlock_();
        field = nullptr;
    }
}
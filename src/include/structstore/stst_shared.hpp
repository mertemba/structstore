#ifndef STST_SHARED_HPP
#define STST_SHARED_HPP

#include "structstore/stst_alloc.hpp"
#include "structstore/stst_structstore.hpp"

#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <random>

namespace structstore {

extern std::random_device rnd_dev;

class not_ready_error : public std::runtime_error {
public:
    not_ready_error(const char* what) : std::runtime_error(what) {}
};

enum CleanupMode {
    NEVER,
    IF_LAST,
    ALWAYS
};

class FD {
    FD(const FD&) = delete;

    FD& operator=(const FD&) = delete;

    int fd;

public:
    FD() : fd(-1) {}

    explicit FD(int fd) : fd(fd) {}

    FD(FD&& other) noexcept: FD() {
        *this = std::move(other);
    }

    FD& operator=(FD&& other) noexcept {
        std::swap(fd, other.fd);
        return *this;
    }

    ~FD() {
        close();
    }

    [[nodiscard]] int get() const {
        return fd;
    }

    void close() {
        if (fd == -1) {
            return;
        }
        ::close(fd);
        fd = -1;
    }

    void release() {
        fd = -1;
    }
};

class StructStoreShared {

    StructStoreShared(const StructStoreShared&) = delete;

    StructStoreShared& operator=(const StructStoreShared&) = delete;

    struct SharedData {
        SharedData* original_ptr;
        size_t size;
        std::atomic_int32_t usage_count;
        MiniMalloc mm_alloc;
        StructStore data;
        std::atomic_bool invalidated;

        SharedData(size_t size, size_t bufsize, void* buffer)
                : original_ptr{this}, size{size}, usage_count{1},
                  mm_alloc{bufsize, buffer}, data{mm_alloc}, invalidated{false} {}

        SharedData() = delete;

        SharedData(SharedData&&) = delete;

        SharedData(const SharedData&) = delete;

        SharedData& operator=(SharedData&&) = delete;

        SharedData& operator=(const SharedData&) = delete;

        ~SharedData() = delete;
    };

    std::string path{};
    FD fd{};
    SharedData* sh_data_ptr{};
    bool use_file{};
    CleanupMode cleanup{};

public:

    explicit StructStoreShared(
            const std::string& path,
            size_t bufsize = 2048,
            bool reinit = false,
            bool use_file = false,
            CleanupMode cleanup = IF_LAST,
            void* target_addr = nullptr);

    explicit StructStoreShared(int fd, bool init);

    StructStoreShared(StructStoreShared&& other) noexcept {
        *this = std::move(other);
    }

    StructStoreShared& operator=(StructStoreShared&& other) noexcept {
        path = std::move(other.path);
        fd = std::move(other.fd);
        sh_data_ptr = other.sh_data_ptr;
        use_file = other.use_file;
        cleanup = other.cleanup;
        other.sh_data_ptr = nullptr;
        other.cleanup = NEVER;
        return *this;
    }

private:

    void mmap_existing_fd();

public:

    bool valid() const {
        return sh_data_ptr != nullptr && !sh_data_ptr->invalidated.load();
    }

    void assert_valid() const {
        if (sh_data_ptr == nullptr) {
            throw std::runtime_error("StructStoreShared instance is invalid");
        }
    }

    bool revalidate(bool block = true);

    StructStore* operator->() {
        assert_valid();
        return &sh_data_ptr->data;
    }

    StructStore& operator*() {
        assert_valid();
        return sh_data_ptr->data;
    }

    FieldAccess<true> operator[](HashString name) {
        assert_valid();
        return sh_data_ptr->data.at(name);
    }

    FieldAccess<true> operator[](const char* name) {
        assert_valid();
        return sh_data_ptr->data[name];
    }

    ~StructStoreShared() {
        STST_LOG_DEBUG() << "deconstructing shared StructStore at " << sh_data_ptr;
        close();
    }

    void close();

    const void* addr() const {
        assert_valid();
        return sh_data_ptr;
    }

    size_t size() const {
        assert_valid();
        return sh_data_ptr->size;
    }

    void to_buffer(void* buffer, size_t bufsize) const;

    void from_buffer(void* buffer, size_t bufsize);

    bool operator==(const StructStoreShared& other) const;

    inline bool operator!=(const StructStoreShared& other) const {
        return !(*this == other);
    }
};

}  // namespace structstore

#endif

#ifndef STRUCTSTORE_SHARED_HPP
#define STRUCTSTORE_SHARED_HPP

#include <atomic>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <random>

namespace structstore {

static std::random_device rnd_dev;

enum CleanupMode {
    NEVER,
    IF_LAST,
    ALWAYS
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

    std::string path;
    int fd;
    SharedData* sh_data_ptr;
    bool use_file;
    CleanupMode cleanup;

public:

    explicit StructStoreShared(
            const std::string& path,
            size_t bufsize = 2048,
            bool reinit = false,
            bool use_file = false,
            CleanupMode cleanup = IF_LAST,
            void* target_addr = nullptr)
        : path(path),
          fd{-1},
          sh_data_ptr{nullptr},
          use_file{use_file},
          cleanup{cleanup}{

        if (use_file) {
            fd = open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
        } else {
            fd = shm_open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
        }

        bool created = fd != -1;

        if (!created) {
            if (use_file) {
                fd = open(path.c_str(), O_RDWR, 0600);
            } else {
                fd = shm_open(path.c_str(), O_RDWR, 0600);
            }
        }

        if (-1 == fd) {
            throw std::runtime_error("opening shared memory failed");
        }

        struct stat fd_state = {};
        fstat(fd, &fd_state);

        if (reinit && fd_state.st_size != 0) {
            // we found an opened memory segment with a non-zero size,
            // it's likely an old segment thus ...

            // ... we open it and mark it as closed ...
            mmap_existing_fd();
            sh_data_ptr->invalidated.store(true);
            sh_data_ptr->usage_count -= 1;

            // ... then unmap it, ...
            munmap(sh_data_ptr, sh_data_ptr->size);
            sh_data_ptr = nullptr;

            // ... then unlink it, ...
            if (use_file) {
                unlink(path.c_str());
            } else {
                shm_unlink(path.c_str());
            }
            close(fd);

            // ... and finally recreate it
            if (use_file) {
                fd = open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
            } else {
                fd = shm_open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
            }

            if (-1 == fd) {
                throw std::runtime_error("opening shared memory failed");
            }
        } else if (!created && fd_state.st_mode != 0100660) {
            // shared memory is not ready for opening yet
            throw std::runtime_error("shared memory not initialized yet");
        }

        size_t size = sizeof(SharedData) + bufsize;

        if (created || reinit) {

            // reserve new memory

            int result = ftruncate(fd, size);
            if (result < 0) {
                throw std::runtime_error("reserving shared memory failed");
            }

            // share memory

            if (target_addr == nullptr) {
                // choose a random address in 48 bit space to avoid collisions
                std::mt19937 rng(rnd_dev());
                std::uniform_int_distribution<std::mt19937::result_type> dist(1, 1 << 30);
                target_addr = (void*) (((uint64_t) dist(rng)) << (47 - 30));
            }
            sh_data_ptr = (SharedData*) mmap(
                    target_addr,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_FIXED_NOREPLACE,
                    fd,
                    0);

            if (sh_data_ptr == MAP_FAILED) {
                throw std::runtime_error("mmap'ing new memory failed");
            }
            if (target_addr != nullptr && sh_data_ptr != target_addr) {
                throw std::runtime_error("mmap'ing new memory to requested address failed");
            }

            // initialize data

            static_assert((sizeof(SharedData) % 8) == 0);
            std::memset(sh_data_ptr, 0, size);
            new(sh_data_ptr) SharedData(size, bufsize, (char*) sh_data_ptr + sizeof(SharedData));

            // marks the store as ready to be used
            fchmod(fd, 0660);

        } else {
            mmap_existing_fd();
        }
    }

    explicit StructStoreShared(int fd, bool init)
            : path{},
              fd{-1},
              sh_data_ptr{nullptr},
              use_file{false},
              cleanup{NEVER} {

        struct stat fd_state = {};
        fstat(fd, &fd_state);
        size_t size = fd_state.st_size;

        if (size == 0) {
            throw std::runtime_error("fstat on fd failed");
        }

        size_t bufsize = size - sizeof(SharedData);

        if (init) {
            // choose a random address in 48 bit space to avoid collisions
            std::mt19937 rng(rnd_dev());
            std::uniform_int_distribution<std::mt19937::result_type> dist(1, 1 << 30);
            void* target_addr = (void*) (((uint64_t) dist(rng)) << (47 - 30));
            // map new memory
            sh_data_ptr = (SharedData*) mmap(
                    target_addr,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED | MAP_FIXED_NOREPLACE,
                    fd,
                    0);

            if (sh_data_ptr == MAP_FAILED) {
                throw std::runtime_error("mmap'ing new memory failed");
            }
            if (sh_data_ptr != target_addr) {
                throw std::runtime_error("mmap'ing new memory to requested address failed");
            }

            // initialize data
            static_assert((sizeof(SharedData) % 8) == 0);
            std::memset(sh_data_ptr, 0, size);
            new(sh_data_ptr) SharedData(size, bufsize, (char*) sh_data_ptr + sizeof(SharedData));
        } else {
            this->fd = fd;
            mmap_existing_fd();
            this->fd = -1;
        }
    }

    StructStoreShared(StructStoreShared&& other) {
        path = std::move(other.path);
        fd = other.fd;
        sh_data_ptr = other.sh_data_ptr;
        use_file = other.use_file;
        cleanup = other.cleanup;
        other.fd = -1;
        other.sh_data_ptr = nullptr;
        other.cleanup = NEVER;
    }

    StructStoreShared& operator=(StructStoreShared&& other) {
        path = std::move(other.path);
        fd = other.fd;
        sh_data_ptr = other.sh_data_ptr;
        use_file = other.use_file;
        cleanup = other.cleanup;
        other.fd = -1;
        other.sh_data_ptr = nullptr;
        other.cleanup = NEVER;
        return *this;
    }

private:

    void mmap_existing_fd() {

        SharedData* original_ptr;

        ssize_t result = read(fd, &original_ptr, sizeof(SharedData*));
        if (result != sizeof(SharedData*)) {
            throw std::runtime_error("reading original pointer failed");
        }

        size_t size;
        result = read(fd, &size, sizeof(size_t));

        if (result != sizeof(size_t)) {
            throw std::runtime_error("reading original size failed");
        }

        if (size < sizeof(SharedData)) {
            throw std::runtime_error("original size is invalid");
        }

        lseek(fd, 0, SEEK_SET);
        sh_data_ptr = (SharedData*) mmap(
                original_ptr,
                size,
                PROT_READ | PROT_WRITE,
                // ensures that the shared memory segment is mapped
                // to the same region of memory for all processes
                // the memory allocator relies on that
                MAP_SHARED | MAP_FIXED_NOREPLACE,
                fd,
                0);

        if (sh_data_ptr == MAP_FAILED || sh_data_ptr != original_ptr) {
            throw std::runtime_error("mmap'ing existing memory failed");
        }

        if (sh_data_ptr->original_ptr != original_ptr) {
            throw std::runtime_error("inconsistency detected");
        }

        ++sh_data_ptr->usage_count;
    }

public:

    bool valid() {

        return sh_data_ptr != nullptr && !sh_data_ptr->invalidated.load();
    }

    bool revalidate(bool block = true) {

        if (valid()) {
            return true;
        }

        // need to revalidate the shared memory segment

        int new_fd = -1;

        do {
            if (new_fd == -1) {
                if (use_file) {
                    new_fd = open(path.c_str(), O_RDWR, 0600);
                } else {
                    new_fd = shm_open(path.c_str(), O_RDWR, 0600);
                }
                if (new_fd == -1) {
                    continue;
                }
            }

            struct stat fd_stat = {};
            fstat(new_fd, &fd_stat);

            // checks if segment is ready
            if (fd_stat.st_mode == 0100660) {

                // unmap as late as possible; in the non-blocking case
                // this keeps the previously mapped memory accessible

                munmap(sh_data_ptr, sh_data_ptr->size);
                sh_data_ptr = nullptr;

                close(fd);

                // open new segment

                fd = new_fd;
                mmap_existing_fd();

                return true;
            }

            // backoff time, while doing busy waiting
            if (block) {
                usleep(1000);
            }

        } while (block);

        // ensures that new_fd is closed
        // necessary in non-blocking mode if segment is not ready yet
        if (new_fd != -1) {
            close(new_fd);
        }

        return false;
    }

    StructStore* operator->() {
        return &sh_data_ptr->data;
    }

    StructStore& operator*() {
        return sh_data_ptr->data;
    }

    explicit operator StructStore&() {
        return sh_data_ptr->data;
    }

    FieldAccess operator[](HashString name) {
        return sh_data_ptr->data[name];
    }

    FieldAccess operator[](const char* name) {
        return sh_data_ptr->data[name];
    }

    ~StructStoreShared() {
        if (sh_data_ptr == nullptr) {
            return;
        }

        if (((--sh_data_ptr->usage_count == 0 && cleanup == IF_LAST) || cleanup == ALWAYS)) {
            bool expected = false;
            // if cleanup == ALWAYS this ensure that unlink is done exactly once
            if (sh_data_ptr->invalidated.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
                if (use_file) {
                    unlink(path.c_str());
                } else {
                    shm_unlink(path.c_str());
                }
            }
        }

        munmap(sh_data_ptr, sh_data_ptr->size);
        if (fd >= 0) {
            close(fd);
        }
    }

    const void* addr() const {
        return sh_data_ptr;
    }

    size_t size() const {
        return sh_data_ptr->size;
    }

    void to_buffer(void* buffer, size_t bufsize) const {
        if (bufsize < sh_data_ptr->size) {
            throw std::runtime_error("target buffer too small");
        }
        std::memcpy(buffer, sh_data_ptr, sh_data_ptr->size);
    }

    void from_buffer(void* buffer, size_t bufsize) {
        if (bufsize < ((SharedData*) buffer)->size) {
            throw std::runtime_error("source buffer too small");
        }
        if (((SharedData*) buffer)->original_ptr != sh_data_ptr) {
            throw std::runtime_error("target address mismatch; please reopen with correct target address");
        }
        std::memcpy(sh_data_ptr, buffer, ((SharedData*) buffer)->size);
    }
};

}  // namespace structstore

#endif

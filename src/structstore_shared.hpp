#ifndef STRUCTSTORE_SHARED_HPP
#define STRUCTSTORE_SHARED_HPP

#include <atomic>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace structstore {

class StructStoreShared {

    StructStoreShared(const StructStoreShared&) = delete;

    StructStoreShared(StructStoreShared&&) = delete;

    StructStoreShared& operator=(const StructStoreShared&) = delete;

    StructStoreShared& operator=(StructStoreShared&&) = delete;

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
    SharedData* ptr;
    bool owning;
    bool persistent;

public:

    explicit StructStoreShared(
            const std::string& path,
            size_t bufsize = 2048,
            bool owning = false,
            bool persistent = false)
        : path(path),
          fd{-1},
          ptr{nullptr},
          owning{owning},
          persistent{persistent} {

        if (persistent) {
            fd = open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
        } else {
            fd = shm_open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
        }

        bool created = fd != -1;

        if (!created) {
            if (persistent) {
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

        if (owning && fd_state.st_size != 0) {
            // we found an opened memory segment with a non-zero size,
            // it's likely an old segment thus ...

            // ... we open it and mark it as closed ...
            mmap_existing_fd();
            ptr->invalidated.store(true);
            ptr->usage_count -= 1;

            // ... then unmap it, ...
            munmap(ptr, ptr->size);
            ptr = nullptr;

            // ... then unlink it, ...
            if (persistent) {
                unlink(path.c_str());
            } else {
                shm_unlink(path.c_str());
            }
            close(fd);

            // ... and finally recreate it
            if (persistent) {
                fd = open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
            } else {
                fd = shm_open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);
            }

            if (-1 == fd) {
                throw std::runtime_error("opening shared memory failed");
            }
        } else if (!created && fd_state.st_mode != 0100660) {
            // shared memory is not ready for opening yet
            return;
        }

        size_t size = sizeof(SharedData) + bufsize;

        if (created || owning) {

            // reserve new memory

            int result = ftruncate(fd, size);
            if (result < 0) {
                throw std::runtime_error("reserving shared memory failed");
            }

            // share memory

            ptr = (SharedData*) mmap(
                    nullptr,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    fd,
                    0);

            if (ptr == MAP_FAILED) {
                throw std::runtime_error("mmap'ing new memory failed");
            }

            // initialize data

            static_assert((sizeof(SharedData) % 8) == 0);
            new(ptr) SharedData(size, bufsize, (char*) ptr + sizeof(SharedData));

            // marks the store as ready to be used
            fchmod(fd, 0660);

        } else {
            mmap_existing_fd();
        }
    }

private:

    void mmap_existing_fd () {

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
        ptr = (SharedData*) mmap(
                original_ptr,
                size,
                PROT_READ | PROT_WRITE,
                // ensures that the shared memory segment is mapped
                // to the same region of memory for all processes
                // the memory allocator relies on that
                MAP_SHARED | MAP_FIXED_NOREPLACE,
                fd,
                0);

        if (ptr == MAP_FAILED || ptr != original_ptr) {
            throw std::runtime_error("mmap'ing existing memory failed");
        }

        if (ptr->original_ptr != original_ptr) {
            throw std::runtime_error("inconsistency detected");
        }

        ++ptr->usage_count;
    }

public:

    bool valid () {

        return !ptr->invalidated.load();
    }

    bool revalidate (bool block = true) {

        if (!ptr->invalidated.load()) {
            return true;
        }

        // need to revalidate the shared memory segment

        int new_fd = -1;

        do {
            if (new_fd == -1) {
                if (persistent) {
                    new_fd = open(path.c_str(), O_RDWR, 0600);
                } else {
                    new_fd = shm_open(path.c_str(), O_RDWR, 0600);
                }
                if (new_fd == -1) {
                    continue;
                }
            }

            struct stat fd_stat = {};
            fstat(fd, &fd_stat);

            // checks if segment is ready
            if (fd_stat.st_mode == 0100660) {

                // unmap as late as possible; in the non-blocking case
                // this keeps the previously mapped memory accessible

                munmap(ptr, ptr->size);
                ptr = nullptr;

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

        return false;
    }

    StructStore* operator->() {
        return &ptr->data;
    }

    StructStore& operator*() {
        return ptr->data;
    }

    explicit operator StructStore&() {
        return shm_ptr->data;
    }

    FieldAccess operator[](HashString name) {
        return ptr->data[name];
    }

    FieldAccess operator[](const char* name) {
        return ptr->data[name];
    }

    ~StructStoreShared() {

        if (owning) {
            ptr->invalidated.store(true);
        }

        size_t usage_count = ptr->usage_count -= 1;

        munmap(ptr, ptr->size);
        ptr = nullptr;

        if (usage_count == 0 || owning) {
            if (!persistent) {
                shm_unlink(path.c_str());
            }
            close(fd);
        }
    }
};

}

#endif

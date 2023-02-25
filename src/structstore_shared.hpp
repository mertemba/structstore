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

    std::string shm_path;
    int shm_fd;
    SharedData* shm_ptr;
    bool owning;

public:

    explicit StructStoreShared(
            const std::string& shm_path,
            size_t bufsize = 2048,
            bool owning = false)
        : shm_path(shm_path),
          shm_fd{-1},
          shm_ptr{nullptr},
          owning{owning} {

        shm_fd = shm_open(shm_path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);

        bool created = shm_fd != -1;

        if (!created) {
            shm_fd = shm_open(shm_path.c_str(), O_RDWR, 0600);
        }

        if (-1 == shm_fd) {
            throw std::runtime_error("opening shared memory failed");
        }

        struct stat shm_stat = {};
        fstat(shm_fd, &shm_stat);

        if (owning && shm_stat.st_size != 0) {
            // we found an opened memory segment with a non-zero size,
            // it's likely an old segment thus ...

            // ... we open it and mark it as closed ...
            mmap_existing_shm();
            shm_ptr->invalidated.store(true);
            shm_ptr->usage_count -= 1;

            // ... then unmap it, ...
            munmap(shm_ptr, shm_ptr->size);
            shm_ptr = nullptr;

            // ... then unlink it, ...
            shm_unlink(shm_path.c_str());
            close(shm_fd);

            // ... and finally recreate it
            shm_fd = shm_open(shm_path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600);

            if (-1 == shm_fd) {
                throw std::runtime_error("opening shared memory failed");
            }
        } else if (!created && shm_stat.st_mode != 0100660) {
            // shared memory is not ready for opening yet
            return;
        }

        size_t size = sizeof(SharedData) + bufsize;

        if (created || owning) {

            // reserve new memory

            int result = ftruncate(shm_fd, size);
            if (result < 0) {
                throw std::runtime_error("reserving shared memory failed");
            }

            // share memory

            shm_ptr = (SharedData*) mmap(
                    nullptr,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_SHARED,
                    shm_fd,
                    0);

            if (shm_ptr == MAP_FAILED) {
                throw std::runtime_error("mmap'ing new memory failed");
            }

            // initialize data

            static_assert((sizeof(SharedData) % 8) == 0);
            new(shm_ptr) SharedData(size, bufsize, (char*) shm_ptr + sizeof(SharedData));

            // marks the store as ready to be used
            fchmod(shm_fd, 0660);

        } else {
            mmap_existing_shm();
        }
    }

private:

    void mmap_existing_shm () {

        SharedData* original_ptr;

        ssize_t result = read(shm_fd, &original_ptr, sizeof(SharedData*));
        if (result != sizeof(SharedData*)) {
            throw std::runtime_error("reading original pointer failed");
        }

        size_t size;
        result = read(shm_fd, &size, sizeof(size_t));

        if (result != sizeof(size_t)) {
            throw std::runtime_error("reading original size failed");
        }

        if (size < sizeof(SharedData)) {
            throw std::runtime_error("original size is invalid");
        }

        lseek(shm_fd, 0, SEEK_SET);
        shm_ptr = (SharedData*) mmap(
                original_ptr,
                size,
                PROT_READ | PROT_WRITE,
                // ensures that the shared memory segment is mapped
                // to the same region of memory for all processes
                // the memory allocator relies on that
                MAP_SHARED | MAP_FIXED_NOREPLACE,
                shm_fd,
                0);

        if (shm_ptr == MAP_FAILED || shm_ptr != original_ptr) {
            throw std::runtime_error("mmap'ing existing memory failed");
        }

        if (shm_ptr->original_ptr != original_ptr) {
            throw std::runtime_error("inconsistency detected");
        }

        ++shm_ptr->usage_count;
    }

public:

    bool valid () {

        return !shm_ptr->invalidated.load();
    }

    void revalidate (bool block = true) {

        if (!shm_ptr->invalidated.load()) {
            return;
        }

        // need to revalidate the shared memory segment

        int new_shm_fd = -1;

        do {
            if (new_shm_fd == -1) {
                new_shm_fd = shm_open(shm_path.c_str(), O_RDWR, 0600);
                if (new_shm_fd == -1) {
                    continue;
                }
            }

            struct stat shm_stat = {};
            fstat(shm_fd, &shm_stat);

            // checks if segment is ready
            if (shm_stat.st_mode == 0100660) {

                // unmap as late as possible; in the non-blocking case
                // this keeps the previously mapped memory accessible

                munmap(shm_ptr, shm_ptr->size);
                shm_ptr = nullptr;

                close(shm_fd);

                // open new segment

                shm_fd = new_shm_fd;
                mmap_existing_shm();

                return;
            }

            // some backoff time, while doing busy waiting
            usleep(1000);

        } while (block);

        throw std::runtime_error("revalidated would block");
    }

    StructStore* operator->() {
        return &shm_ptr->data;
    }

    StructStore& operator*() {
        return shm_ptr->data;
    }

    FieldAccess operator[](HashString name) {
        return shm_ptr->data[name];
    }

    FieldAccess operator[](const char* name) {
        return shm_ptr->data[name];
    }

    ~StructStoreShared() {

        if (owning) {
            shm_ptr->invalidated.store(true);
        }

        size_t usage_count = shm_ptr->usage_count -= 1;

        munmap(shm_ptr, shm_ptr->size);
        shm_ptr = nullptr;

        if (usage_count == 0 || owning) {
            shm_unlink(shm_path.c_str());
            close(shm_fd);
        }
    }
};

}

#endif

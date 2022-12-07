#ifndef STRUCTSTORE_SHARED_HPP
#define STRUCTSTORE_SHARED_HPP

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

        SharedData(size_t size, size_t bufsize, void* buffer)
                : original_ptr{this}, size{size}, usage_count{1},
                  mm_alloc{bufsize, buffer}, data{mm_alloc} {}

        SharedData() = delete;

        SharedData(SharedData&&) = delete;

        SharedData(const SharedData&) = delete;

        SharedData& operator=(SharedData&&) = delete;

        SharedData& operator=(const SharedData&) = delete;

        ~SharedData() = delete;
    };

    int shm_fd;
    std::string shm_path;
    SharedData* shm_ptr;

public:
    explicit StructStoreShared(const std::string& shm_path, size_t bufsize = 2048) : shm_path(shm_path) {
        shm_fd = shm_open(shm_path.c_str(), O_CREAT | O_RDWR, 0600);
        size_t size = sizeof(SharedData) + bufsize;

        // check if shared memory already exists
        struct stat shm_stat = {};
        fstat(shm_fd, &shm_stat);
        if (shm_stat.st_size > 0) {
            std::cout << "using existing memory ..." << std::endl;
            // use existing memory
            SharedData* original_ptr;
            ssize_t result = read(shm_fd, &original_ptr, sizeof(SharedData*));
            if (result != sizeof(SharedData*)) {
                throw std::runtime_error("reading original pointer failed");
            }
            result = read(shm_fd, &size, sizeof(size_t));
            if (result != sizeof(size_t)) {
                throw std::runtime_error("reading original size failed");
            }
            if (size < sizeof(SharedData)) {
                throw std::runtime_error("original size is invalid");
            }
            lseek(shm_fd, 0, SEEK_SET);
            shm_ptr = (SharedData*) mmap(original_ptr, size, PROT_READ | PROT_WRITE,
                                         MAP_SHARED | MAP_FIXED_NOREPLACE, shm_fd, 0);
            if (shm_ptr == MAP_FAILED || shm_ptr != original_ptr) {
                throw std::runtime_error("mmap'ing existing memory failed");
            }
            if (shm_ptr->original_ptr != original_ptr) {
                throw std::runtime_error("inconsistency detected");
            }
            ++shm_ptr->usage_count;
        } else {
            std::cout << "reserving new memory ..." << std::endl;
            // reserve new memory
            int result = ftruncate(shm_fd, size);
            if (result < 0) {
                throw std::runtime_error("reserving shared memory failed");
            }
            // share memory
            shm_ptr = (SharedData*) mmap(nullptr, size, PROT_READ | PROT_WRITE,
                                         MAP_SHARED, shm_fd, 0);
            if (shm_ptr == MAP_FAILED) {
                throw std::runtime_error("mmap'ing new memory failed");
            }
            // initialize data
            static_assert((sizeof(SharedData) % 8) == 0);
            new(shm_ptr) SharedData(size, bufsize, (char*) shm_ptr + sizeof(SharedData));
        }
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
        if (--shm_ptr->usage_count == 0) {
            // unlink shared memory
            shm_unlink(shm_path.c_str());
            close(shm_fd);
        }
    }
};

}

#endif

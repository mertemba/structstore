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
        Arena arena;
        StructStore data;

        SharedData() = delete;
    };

    int shm_fd;
    std::string shm_path;
    SharedData* shm_ptr;

public:
    explicit StructStoreShared(const std::string& shm_path, ssize_t bufsize = 1024) : shm_path(shm_path) {
        shm_fd = shm_open(shm_path.c_str(), O_CREAT | O_RDWR, 0600);
        ssize_t size = sizeof(SharedData) + bufsize;

        // check if shared memory already exists
        struct stat shm_stat = {0};
        fstat(shm_fd, &shm_stat);
        if (shm_stat.st_size > 0) {
            std::cout << "using existing memory ..." << std::endl;
            // use existing memory
            SharedData* original_ptr;
            ssize_t result = read(shm_fd, &original_ptr, sizeof(SharedData*));
            if (result != sizeof(SharedData*)) {
                throw std::runtime_error("reading original pointer failed");
            }
            shm_ptr = (SharedData*) mmap(original_ptr, size, PROT_READ | PROT_WRITE,
                                         MAP_SHARED | MAP_FIXED_NOREPLACE, shm_fd, 0);
            if (shm_ptr == MAP_FAILED || shm_ptr != original_ptr) {
                throw std::runtime_error("mmap'ing existing memory failed");
            }
            if (shm_ptr->original_ptr != original_ptr) {
                throw std::runtime_error("inconsistency detected");
            }
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
            shm_ptr->original_ptr = shm_ptr;
            // add memory buffer
            shm_ptr->arena = Arena(bufsize, (char*) shm_ptr + sizeof(SharedData));
            // initialize data
            new(&shm_ptr->data) StructStore(shm_ptr->arena);
        }
    }

    StructStore* operator->() {
        return &shm_ptr->data;
    }

    StructStore& operator*() {
        return shm_ptr->data;
    }

    StructStoreAccess operator[](HashString name) {
        return shm_ptr->data[name];
    }

    StructStoreAccess operator[](const char* name) {
        return shm_ptr->data[name];
    }

    ~StructStoreShared() {
        shm_unlink(shm_path.c_str());
        close(shm_fd);
    }
};

}

#endif

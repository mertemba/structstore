#include "structstore/stst_shared.hpp"
#include "structstore/stst_alloc.hpp"

using namespace structstore;

std::random_device structstore::rnd_dev;

StructStoreShared::StructStoreShared(
        const std::string& path,
        size_t bufsize,
        bool reinit,
        bool use_file,
        CleanupMode cleanup,
        void* target_addr)
        : path(path),
          fd{-1},
          sh_data_ptr{nullptr},
          use_file{use_file},
          cleanup{cleanup} {

    if (use_file) {
        fd = FD(open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600));
    } else {
        fd = FD(shm_open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600));
    }

    bool created = fd.get() != -1;

    if (!created) {
        if (use_file) {
            fd = FD(open(path.c_str(), O_RDWR, 0600));
        } else {
            fd = FD(shm_open(path.c_str(), O_RDWR, 0600));
        }
    }

    if (fd.get() == -1) {
        throw std::runtime_error("opening shared memory failed");
    }

    struct stat fd_state = {};
    fstat(fd.get(), &fd_state);

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

        // ... and finally recreate it
        if (use_file) {
            fd = FD(open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600));
        } else {
            fd = FD(shm_open(path.c_str(), O_EXCL | O_CREAT | O_RDWR, 0600));
        }

        if (fd.get() == -1) {
            throw std::runtime_error("opening shared memory failed");
        }
    } else if (!created && fd_state.st_mode == 0100600) {
        // shared memory is not ready for opening yet
        throw not_ready_error("shared memory not initialized yet");
    } else if (!created && fd_state.st_mode != 0100660) {
        // shared memory is somehow uninitialized
        throw std::runtime_error("shared memory not initialized");
    }

    size_t size = sizeof(SharedData) + bufsize;

    if (created || reinit) {

        // reserve new memory

        int result = ftruncate(fd.get(), size);
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
        auto map_flags = MAP_SHARED | MAP_FIXED_NOREPLACE;
        #ifdef __SANITIZE_ADDRESS__
        target_addr = nullptr;
        map_flags = MAP_SHARED;
        #endif
        sh_data_ptr = (SharedData*) mmap(
                target_addr,
                size,
                PROT_READ | PROT_WRITE,
                map_flags,
                fd.get(),
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
        STST_LOG_DEBUG() << "created shared StructStore at " << sh_data_ptr;

        // marks the store as ready to be used
        fchmod(fd.get(), 0660);

    } else {
        mmap_existing_fd();
        STST_LOG_DEBUG() << "opened shared StructStore at " << sh_data_ptr;
    }
}

StructStoreShared::StructStoreShared(int fd, bool init)
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
        auto map_flags = MAP_SHARED | MAP_FIXED_NOREPLACE;
        #ifdef __SANITIZE_ADDRESS__
        target_addr = nullptr;
        map_flags = MAP_SHARED;
        #endif
        sh_data_ptr = (SharedData*) mmap(
                target_addr,
                size,
                PROT_READ | PROT_WRITE,
                map_flags,
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
        this->fd = FD(fd);
        mmap_existing_fd();
        this->fd.release();
    }
}


void StructStoreShared::mmap_existing_fd() {

    SharedData* original_ptr;

    ssize_t result = read(fd.get(), &original_ptr, sizeof(SharedData*));
    if (result != sizeof(SharedData*)) {
        throw std::runtime_error("reading original pointer failed");
    }

    size_t size;
    result = read(fd.get(), &size, sizeof(size_t));

    if (result != sizeof(size_t)) {
        throw std::runtime_error("reading original size failed");
    }

    if (size < sizeof(SharedData)) {
        throw std::runtime_error("original size is invalid");
    }

    lseek(fd.get(), 0, SEEK_SET);
    sh_data_ptr = (SharedData*) mmap(
            original_ptr,
            size,
            PROT_READ | PROT_WRITE,
            // ensures that the shared memory segment is mapped
            // to the same region of memory for all processes
            // the memory allocator relies on that
            MAP_SHARED | MAP_FIXED_NOREPLACE,
            fd.get(),
            0);

    if (sh_data_ptr == MAP_FAILED || sh_data_ptr != original_ptr) {
        throw std::runtime_error("mmap'ing existing memory failed");
    }

    if (sh_data_ptr->original_ptr != original_ptr) {
        throw std::runtime_error("inconsistency detected");
    }

    ++sh_data_ptr->usage_count;

#ifndef NDEBUG
    sh_data_ptr->data.check(sh_data_ptr->mm_alloc, nullptr);
#endif
}

bool StructStoreShared::revalidate(bool block) {

    if (valid()) {
        return true;
    }

    // need to revalidate the shared memory segment

    FD new_fd;

    do {
        if (new_fd.get() == -1) {
            if (use_file) {
                new_fd = FD(open(path.c_str(), O_RDWR, 0600));
            } else {
                new_fd = FD(shm_open(path.c_str(), O_RDWR, 0600));
            }
            if (new_fd.get() == -1) {
                continue;
            }
        }

        struct stat fd_stat = {};
        fstat(new_fd.get(), &fd_stat);

        // checks if segment is ready
        if (fd_stat.st_mode == 0100660) {

            // unmap as late as possible; in the non-blocking case
            // this keeps the previously mapped memory accessible

            munmap(sh_data_ptr, sh_data_ptr->size);
            sh_data_ptr = nullptr;

            // open new segment

            fd = std::move(new_fd);
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


void StructStoreShared::close() {
    if (sh_data_ptr == nullptr) {
        return;
    }

    if (((--sh_data_ptr->usage_count == 0 && cleanup == IF_LAST) || cleanup == ALWAYS)) {
        bool expected = false;
        // if cleanup == ALWAYS this ensure that unlink is done exactly once
        if (sh_data_ptr->invalidated.compare_exchange_strong(expected, true, std::memory_order_acquire)) {
            sh_data_ptr->data.~StructStore();
            sh_data_ptr->mm_alloc.~MiniMalloc();
            if (use_file) {
                unlink(path.c_str());
            } else {
                shm_unlink(path.c_str());
            }
        }
    }

    munmap(sh_data_ptr, sh_data_ptr->size);
    sh_data_ptr = nullptr;
}

void StructStoreShared::to_buffer(void* buffer, size_t bufsize) const {
    assert_valid();
    if (bufsize < sh_data_ptr->size) {
        throw std::runtime_error("target buffer too small");
    }
    std::memcpy(buffer, sh_data_ptr, sh_data_ptr->size);
}

void StructStoreShared::from_buffer(void* buffer, size_t bufsize) {
    assert_valid();
    if (bufsize < ((SharedData*) buffer)->size) {
        throw std::runtime_error("source buffer too small");
    }
    if (((SharedData*) buffer)->original_ptr != sh_data_ptr) {
        throw std::runtime_error("target address mismatch; please reopen with correct target address");
    }
    std::memcpy(sh_data_ptr, buffer, ((SharedData*) buffer)->size);
}

bool StructStoreShared::operator==(const StructStoreShared& other) const {
    assert_valid();
    other.assert_valid();
    return sh_data_ptr->data == other.sh_data_ptr->data;
}

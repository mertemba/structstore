#ifndef STST_UTILS_HPP
#define STST_UTILS_HPP

#include <cstdint>
#include <iostream>
#include <sstream>

namespace structstore {

class Log {
public:
    enum class Level {
        DEBUG,
        INFO,
        WARN,
        ERROR
    };
    static Level level;

private:
    mutable std::ostringstream str;

public:
    explicit Log(const char* prefix) { str << prefix; }

    template<typename T>
    friend const Log& operator<<(const Log& log, const T& t) {
        log.str << t;
        return log;
    }

    ~Log() {
        str << '\n';
        std::cout << str.str();
#ifndef NDEBUG
        std::cout << std::flush;
#endif
    }
};

class NilLog {
    template<typename T>
    inline friend const NilLog& operator<<(const NilLog& log, const T&) { return log; }
};

#ifdef NDEBUG
#define STST_LOG_DEBUG() NilLog()
#define STST_LOG_INFO() NilLog()
#else
#define STST_LOG_DEBUG()                                                                           \
    if (structstore::Log::Level::DEBUG >= structstore::Log::level) structstore::Log("debug: ")
#define STST_LOG_INFO()                                                                            \
    if (structstore::Log::Level::INFO >= structstore::Log::level) structstore::Log("info: ")
#endif

#define STST_LOG_WARN()                                                                            \
    if (structstore::Log::Level::WARN >= structstore::Log::level) structstore::Log("warning: ")
#define STST_LOG_ERROR()                                                                           \
    if (structstore::Log::Level::ERROR >= structstore::Log::level) structstore::Log("error: ")

#define stst_assert(expr)                                                                          \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            structstore::Callstack::throw_with_trace<std::runtime_error>(                          \
                    "assertion failed: " #expr);                                                   \
        }                                                                                          \
    } while (0)

constexpr uint64_t const_hash(const char* input) {
    // FNV1a hash of reversed string
    return *input != 0 ? (const_hash(input + 1) ^ uint64_t((uint8_t) *input)) * 0x100000001b3ull
                       : 0xcbf29ce484222325ull;
}

} // namespace structstore

#endif
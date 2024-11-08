#ifndef STST_UTILS_HPP
#define STST_UTILS_HPP

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

#define try_with_info(info_stream, stmt)                               \
    do {                                                               \
        try {                                                          \
            STST_LOG_DEBUG() << "checking " << info_stream;            \
            stmt                                                       \
        } catch (std::runtime_error & _e) {                            \
            STST_LOG_DEBUG() << info_stream << "error: " << _e.what(); \
            std::ostringstream _str;                                   \
            _str << info_stream << _e.what();                          \
            throw std::runtime_error(_str.str());                      \
        }                                                              \
    } while (0)

} // namespace structstore

#endif
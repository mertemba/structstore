// This file is part of the StructStore library.
// Copyright (C) 2022-2025 Max Mertens
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License v3.0
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU General Lesser Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
#define STST_LOG_DEBUG() structstore::NilLog()
#define STST_LOG_INFO() structstore::NilLog()
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
        if (!(expr)) { structstore::Callstack::throw_with_trace("assertion failed: " #expr); }     \
    } while (0)

constexpr uint32_t const_hash(const char* input) {
    // FNV1a hash of reversed string
    return *input != 0 ? (const_hash(input + 1) ^ uint32_t((uint8_t) *input)) * 0x01000193ul
                       : 0x811C9DC5ul;
}

} // namespace structstore

#endif

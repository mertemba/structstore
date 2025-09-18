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

#ifndef STST_CALLSTACK_HPP
#define STST_CALLSTACK_HPP

#include <stdexcept>
#include <string>
#include <vector>

namespace structstore {

class Callstack {
    std::vector<std::string> entries;

public:
    static Callstack& cur();

    void push(const std::string& what);

    void pop();

    std::string format_with_trace(const std::string& what);

    template<typename T = std::runtime_error>
    static void throw_with_trace(const std::string& what) {
        throw T(cur().format_with_trace(what));
    }

    static void warn_with_trace(const std::string& what);
};

struct CallstackEntry {
    CallstackEntry() = delete;

#ifndef NDEBUG
    inline CallstackEntry(const std::string& what) { Callstack::cur().push(what); }
    inline ~CallstackEntry() { Callstack::cur().pop(); }
#else
    inline CallstackEntry(const std::string&) {}
    inline ~CallstackEntry() {}
#endif
};

} // namespace structstore

#endif
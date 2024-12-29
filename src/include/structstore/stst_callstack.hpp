#ifndef STST_CALLSTACK_HPP
#define STST_CALLSTACK_HPP

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

    template<typename T>
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
#ifndef STST_CALLSTACK_HPP
#define STST_CALLSTACK_HPP

#include <string>
#include <vector>

namespace structstore {

class Callstack {
    std::vector<const char*> entries;

public:
    static Callstack& cur();

    void push(const char* what);

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
    inline CallstackEntry(const char* what) { Callstack::cur().push(what); }
    inline ~CallstackEntry() { Callstack::cur().pop(); }
#else
    inline CallstackEntry(const char*) {}
    inline ~CallstackEntry() {}
#endif
};

} // namespace structstore

#endif
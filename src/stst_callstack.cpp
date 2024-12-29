#include "structstore/stst_callstack.hpp"
#include <iostream>
#include <sstream>

using namespace structstore;

static thread_local Callstack cur_stack;

Callstack& Callstack::cur() { return cur_stack; }

void Callstack::push(const std::string& what) { entries.push_back(what); }

void Callstack::pop() { entries.pop_back(); }

std::string Callstack::format_with_trace(const std::string& what) {
    std::ostringstream str;
    for (const std::string& entry: entries) { str << entry << ": "; }
    str << what;
    return str.str();
}

void Callstack::warn_with_trace(const std::string& what) {
    std::cerr << "warning: " << cur_stack.format_with_trace(what) << std::endl;
}
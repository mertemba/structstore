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
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

#ifndef MINI_MALLOC_HPP
#define MINI_MALLOC_HPP

#include <cstddef>

namespace structstore {

struct mini_malloc;

// this function must be called exactly once before the first call to sh_alloc or mm_free,
// with a block of memory and its size as parameters
void init_mini_malloc(mini_malloc* sh_alloc, size_t blocksize);

// returns a pointer to size bytes of memory, aligned to 8 bytes
void* mm_allocate(mini_malloc* sh_alloc, size_t size);

// free a block of memory previously allocated by sh_alloc
void mm_free(mini_malloc* sh_alloc, const void* ptr);

void mm_assert_all_freed(mini_malloc* sh_alloc);

} // namespace structstore

#endif

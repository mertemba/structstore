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

#include "structstore/stst_structstore.hpp"
#include "structstore/stst_callstack.hpp"

using namespace structstore;

const TypeInfo& StructStore::type_info =
        typing::register_type<StructStore>("structstore::StructStore");

void StructStore::check(const SharedAlloc* sh_alloc) const {
    CallstackEntry entry{"structstore::StructStore::check()"};
    field_map.check(sh_alloc, *this);
}

FieldAccess<true> StructStore::at(const std::string& name) {
    return FieldAccess<true>{field_map.at(name), field_map.get_alloc(), this};
}

FieldAccess<true> StructStore::operator[](const std::string& name) {
    return FieldAccess<true>{field_map[name], field_map.get_alloc(), this};
}

#ifndef EXAMPLE_HPP
#define EXAMPLE_HPP

#include <structstore.hpp>

namespace stst = structstore;

struct Subsettings : stst::StructStore<Subsettings> {
    int subnum = 42;
    stst::arena_str substr = {"bar", alloc};

    Subsettings() : StructStore<Subsettings>() {}

    void list_fields() {
        register_field("subnum", subnum);
        register_field("substr", substr);
    }
};

struct Settings : stst::StructStore<Settings> {
    int num = 5;
    bool flag = true;
    stst::arena_str str = {"foo", alloc};
    Subsettings subsettings{};

    void list_fields() {
        register_field("num", num);
        register_field("flag", flag);
        register_field("str", str);
        register_field("subsettings", subsettings);
    }
};

#endif

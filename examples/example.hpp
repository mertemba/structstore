#ifndef EXAMPLE_HPP
#define EXAMPLE_HPP

#include "structstore.hpp"

struct Subsettings : StructStore<Subsettings> {
    friend class StructStore<Subsettings>;

    int subnum = 42;
    arena_str substr = {"bar", alloc};

    Subsettings() : StructStore<Subsettings>() {}

private:
    void list_fields() {
        register_field("subnum", subnum);
        register_field("substr", substr);
    }
};

struct Settings : StructStore<Settings> {
    friend class StructStore<Settings>;

    int num = 5;
    bool flag = true;
    arena_str str = {"foo", alloc};
    Subsettings subsettings{};

private:
    void list_fields() {
        register_field("num", num);
        register_field("flag", flag);
        register_field("str", str);
        register_field("subsettings", subsettings);
    }
};

#endif

#include <iostream>
#include "example.hpp"
#include <structstore_shared.hpp>

int main() {
    Settings settings;
    std::cout << "settings: " << settings << std::endl;
    std::cout << YAML::Dump(to_yaml(settings)) << std::endl;

    StructStoreShared<Settings> shdata("/shdata_store");
    std::cout << "got data" << std::endl;
    std::cout << "str: " << shdata->str << std::endl;
    std::cout << "shared data: " << *shdata << std::endl;
    std::cout << "shared data alloc: " << shdata->allocated_size() << std::endl;
    shdata->num = 77;
    shdata->subsettings.subnum = 77;
    return 0;
}

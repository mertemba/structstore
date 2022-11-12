#include <iostream>
#include "example.hpp"
#include <structstore_shared.hpp>

int main() {
    Settings settings;
    std::cout << "settings: " << settings << std::endl;
    std::cout << YAML::Dump(to_yaml(settings)) << std::endl;

    stst::StructStoreShared<Settings> shdata("/shdata_store", 4096);
    std::cout << "shared data: " << *shdata << std::endl;
    std::cout << "allocated bytes: " << shdata->allocated_size() << std::endl;
    return 0;
}

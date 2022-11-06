#include <iostream>
#include "example.hpp"

int main() {
    Settings settings;
    std::cout << "Settings: " << settings << std::endl;
    std::cout << YAML::Dump(to_yaml(settings)) << std::endl;
    return 0;
}

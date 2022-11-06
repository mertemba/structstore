#include <iostream>
#include "structstore_dyn.hpp"

int main() {
    StructStoreDyn state;
    int& num = state.add_field<int>("num");
    num = 5;
    std::cout << "State: " << state << std::endl;
    std::cout << YAML::Dump(to_yaml(state)) << std::endl;
    state["num"] = 42;
    std::cout << "num is " << (int) state["num"] << std::endl;
    return 0;
}

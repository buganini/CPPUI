#include "virtual_nodes.hpp"
#include <iostream>

int main(int argc, char ** argv) {
    try {
        Examples::VirtualNodes::App app(argc, argv);
        app.run();
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

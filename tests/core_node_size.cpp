#include "cppui/node.hpp"

struct Node:CPPUI::Node {void destroy(bool) override {}};
int main() {
    Node node;
    return node.size().get() ? 1 : 0;
}

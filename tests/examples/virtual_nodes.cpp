#include "example/virtual_nodes.hpp"
#include "backend.hpp"
#include <iostream>

int main(int argc, char ** argv) {
    using namespace ExampleTest;
    try {
        Examples::VirtualNodes::App app(argc, argv);
        auto & state = Examples::VirtualNodes::model();
        app.sync();
        state.count1 = 3;
        state.count3 = 2;
        settle();
        require(hasText(app, "View1-2") && hasText(app, "View3-1"), "independent view updates were lost");
        for(int i = 1; i <= 3; ++i) {
            require(click(app, "Hide"), "hide button missing");
            settle();
            require(!state.show2 && !hasText(app, "View2Sub-0"), "virtual subtree was not removed");
            state.count2sub = i;
            require(click(app, "Show"), "show button missing");
            settle();
            require(state.show2 && hasText(app, "View2Sub-" + std::to_string(i - 1)),
                    "reinserted nested view lost its current state");
        }
        state.count2sub = 0;
        settle();
        require(!hasText(app, "View2Sub-0") && hasText(app, "View3-1"),
                "empty nested view disturbed its sibling");
        state.count2sub = 2;
        settle();
        require(hasText(app, "View2Sub-1") && hasText(app, "View3-1"),
                "nested view could not grow from an empty range");
        std::cout << CPPUI::Backend::name << " virtual-node example smoke passed\n";
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

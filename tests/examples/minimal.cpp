#include "example/minimal.hpp"
#include "backend.hpp"
#include <iostream>

int main(int argc, char ** argv) {
    using namespace ExampleTest;
    try {
        Examples::Counter app(argc, argv);
        app.sync();
        settle();
        for(int i = 0; i < 3; ++i) {
            require(click(app, "+"), "increment button missing");
            settle();
        }
        auto counter = find(app, "counter");
        auto labels = find(app, "labels");
        require(app.n == 4 && counter && hasText(*counter, "4") && labels && labels->children->size() == 4,
                "counter or repeated labels did not increment");
        auto checkbox = find(app, "show-labels");
        require(checkbox && toggleCheckbox(*checkbox), "show-labels checkbox missing");
        settle();
        require(!app.showLabels && !find(app, "labels"), "checkbox did not remove the label row");
        checkbox = find(app, "show-labels");
        require(checkbox && toggleCheckbox(*checkbox), "checkbox lost after redraw");
        settle();
        labels = find(app, "labels");
        require(app.showLabels && labels && labels->children->size() == 4, "label row did not remount");
        for(int i = 0; i < 5; ++i) {
            require(click(app, "-"), "decrement button missing");
            settle();
        }
        counter = find(app, "counter");
        labels = find(app, "labels");
        require(app.n == -1 && counter && hasText(*counter, "-1") && labels && labels->children->empty(),
                "empty counter range did not redraw");
        std::cout << CPPUI::Backend::name << " counter example smoke passed\n";
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

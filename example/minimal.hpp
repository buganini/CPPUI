#ifndef CPPUI_EXAMPLE_MINIMAL
#define CPPUI_EXAMPLE_MINIMAL
#include "cppui/backend.hpp"

namespace Examples {
    class Counter: public CPPUI::Application {
        public:
        using Application::Application;
        CPPUI::State<int> n{1};
        CPPUI::State<bool> showLabels{true};

        void content() override {
            using namespace CPPUI;
            for(auto window: CPPUI::Window().title("CPPUI counter").id("window")) {
                for(auto column: VBox().layout(480, 320, {}, Insets(12))) {
                    for(auto row: HBox()) {
                        Button("-").click([this]() { n -= 1; });
                        Label(std::to_string(n)).layout(60).style(0x336699, {}, 20).id("counter");
                        Button("+").click([this]() { n += 1; });
                    }
                    Checkbox("Show labels", showLabels).id("show-labels");
                    if(showLabels) for(auto row: HBox().id("labels")) {
                        for(int i = 0; i < n; ++i) Label(std::to_string(i));
                    }
                    Divider();
                    Text("The counter updates the labels above. This text wraps to the available width.");
                    Spacer();
                }
            }
        }
    };
}
#endif

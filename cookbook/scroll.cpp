#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void scroll(Model & model) {
        for(auto column: Presentation::VBox(1).id("result-scroller-container")) {
            for(auto row: Presentation::HBox()) {
                CPPUI::Button("-").click([&model]() { model.scrollRows -= 1; });
                Presentation::Label(std::to_string(static_cast<int>(model.scrollRows)));
                CPPUI::Button("+").click([&model]() { model.scrollRows += 1; });
            }
            for(auto scroll: CPPUI::Scroll().scrollY(CPPUI::ScrollEnd).id("scroller")) {
                for(auto content: Presentation::VBox().id("scrolled-content")) {
                    for(int i = 0; i < model.scrollRows; ++i) Presentation::Label("Row " + std::to_string(i + 1));
                }
            }
        }
    }
}

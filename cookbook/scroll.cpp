#include "cppui/backend.hpp"
#include "recipes.hpp"

#if CPPUI_HAS_SCROLL_ANCHOR
namespace Cookbook {
    void scroll(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1).id("result-scroller-container")) {
            for(auto row: CPPUI::HBox()) {
                CPPUI::Button("-").click([&model]() { model.scrollRows -= 1; });
                CPPUI::Label(std::to_string(static_cast<int>(model.scrollRows)));
                CPPUI::Button("+").click([&model]() { model.scrollRows += 1; });
            }
            for(auto scroll: CPPUI::Scroll().scrollY(CPPUI::ScrollEnd).id("scroller")) {
                for(auto content: CPPUI::VBox().id("scrolled-content")) {
                    for(int i = 0; i < model.scrollRows; ++i) CPPUI::Label("Row " + std::to_string(i + 1));
                }
            }
        }
    }
}
#endif

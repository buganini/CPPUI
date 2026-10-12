#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void mdi(Model &) {
        for(auto area: CPPUI::MdiArea()) {
            for(auto first: CPPUI::MdiSubWindow()) for(auto column: Presentation::VBox()) {
                Presentation::Label("1");
                Presentation::Label("2");
            }
            for(auto second: CPPUI::MdiSubWindow()) for(auto column: Presentation::VBox()) {
                Presentation::Label("A");
                Presentation::Label("B");
            }
        }
    }
}

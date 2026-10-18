#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void stack(Model &) {
        for(auto column: CPPUI::VBox()) {
            for(auto layers: CPPUI::Stack().id("example-stack")) {
                CPPUI::Label("Bottom").style({}, {}, 48).id("stack-bottom");
                CPPUI::Label("Top").style(0xff0000, {}, 32).id("stack-top");
            }
            CPPUI::Spacer();
        }
    }
}

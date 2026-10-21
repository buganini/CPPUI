#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void stack(Model &) {
        for(auto column: CPPUI::VBox()) {
            for(auto layers: CPPUI::Stack().id("example-stack")) {
                CPPUI::Label("Bottom").style(0x244B5A, {}, 48).id("stack-bottom");
                CPPUI::Label("Top").style(0xA84A3A, {}, 32).id("stack-top");
            }
            CPPUI::Spacer();
        }
    }
}

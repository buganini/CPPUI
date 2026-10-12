#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void stack(Model &) {
        for(auto column: CPPUI::VBox()) {
            for(auto layers: CPPUI::Stack()) {
                CPPUI::Label("Bottom").style({}, {}, 48);
                CPPUI::Label("Top").style(0xff0000, {}, 32);
            }
            CPPUI::Spacer();
        }
    }
}

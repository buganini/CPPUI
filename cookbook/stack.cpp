#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void stack(Model &) {
        for(auto column: Presentation::VBox()) {
            for(auto layers: CPPUI::Stack()) {
                Presentation::StackLabel("Bottom", 48, false);
                Presentation::StackLabel("Top", 32, true);
            }
            Presentation::Spacer();
        }
    }
}

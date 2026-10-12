#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void splitter(Model &) {
        for(auto vertical: CPPUI::Splitter(true)) {
            for(auto horizontal: CPPUI::Splitter()) {
                for(auto column: Presentation::VBox()) {
                    Presentation::Label("item 1");
                    Presentation::Label("item 2");
                    Presentation::Label("item 3");
                    Presentation::Label("item 4");
                }
                Presentation::GrayLabel("pane");
            }
            Presentation::GrayText("blah blah");
        }
    }
}

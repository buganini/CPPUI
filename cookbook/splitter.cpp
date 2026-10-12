#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void splitter(Model &) {
        for(auto vertical: CPPUI::Splitter(true)) {
            for(auto horizontal: CPPUI::Splitter()) {
                for(auto column: CPPUI::VBox()) {
                    CPPUI::Label("item 1");
                    CPPUI::Label("item 2");
                    CPPUI::Label("item 3");
                    CPPUI::Label("item 4");
                }
                CPPUI::Label("pane").style({}, 0x808080);
            }
            CPPUI::Text("blah blah").style({}, 0x808080);
        }
    }
}

#include "backend.hpp"
#include "recipes.hpp"

#if !defined(CPPUI_COOKBOOK_LVGL)
namespace Cookbook {
    void splitter(Model &) {
        for(auto vertical: CPPUI::Splitter(true).id("example-splitter")) {
            for(auto horizontal: CPPUI::Splitter().id("splitter-horizontal")) {
                for(auto column: CPPUI::VBox()) {
                    CPPUI::Label("item 1");
                    CPPUI::Label("item 2");
                    CPPUI::Label("item 3");
                    CPPUI::Label("item 4");
                }
                CPPUI::Label("pane 2").style({}, 0xCCCCCC);
                CPPUI::Label("pane 3").style({}, 0xDDDDDD);
            }
            CPPUI::Text("blah blah").style({}, 0xCCCCCC);
        }
    }
}
#endif

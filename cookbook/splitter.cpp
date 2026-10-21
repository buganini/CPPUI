#include "cppui/backend.hpp"
#include "recipes.hpp"

#if CPPUI_HAS_SPLITTER
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
                CPPUI::Label("pane 2").style(0xFFF7E6, 0x244B5A);
                CPPUI::Label("pane 3").style(0x1C2A30, 0xD3B98B);
            }
            CPPUI::Text("blah blah").style(0x1C2A30, 0xF2E6CF);
        }
    }
}
#endif

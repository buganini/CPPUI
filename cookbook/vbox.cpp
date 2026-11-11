#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void vbox(Model & model) {
        for(auto column: CPPUI::VBox().layout(CPPUI::layout::weight(1)).id("spacer-column")) {
            CPPUI::Checkbox("Reverse vertical order",model.inverseV).id("vbox-inverse");
            for(auto reversed:CPPUI::VBox(model.inverseV).layout(CPPUI::layout::weight(1))) {
                CPPUI::Label("Row 1");
                CPPUI::Divider();
                CPPUI::Label("Row 2");
            }
            CPPUI::Divider();
            CPPUI::Label("Default gap: 10 pixels");
            for(auto box:CPPUI::VBox(model.inverseV)) {
                CPPUI::Label("First"); CPPUI::Label("Second"); CPPUI::Label("Third");
            }
            CPPUI::Divider();
            CPPUI::Label("gap=12 pixels");
            for(auto box:CPPUI::VBox(model.inverseV,12)) {
                CPPUI::Label("First"); CPPUI::Label("Second"); CPPUI::Label("Third");
            }
            CPPUI::Spacer().id("column-gap");
        }
    }
}

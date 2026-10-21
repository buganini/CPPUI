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
            CPPUI::Spacer().id("column-gap");
        }
    }
}

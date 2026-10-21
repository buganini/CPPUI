#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void label(Model & model) {
        for(auto column: CPPUI::VBox().layout(CPPUI::layout::weight(1))) {
            CPPUI::Checkbox("Show greeting",model.greetingVisible);
            CPPUI::Label("Hello World").visible(model.greetingVisible);
            CPPUI::Label("Color")
                .style(CPPUI::style::color(0xD3B98B), CPPUI::style::bgColor(0x1C2A30), CPPUI::style::fontSize(20));
            CPPUI::Label("No <b>HTML</b>");
            CPPUI::Spacer();
        }
    }
}

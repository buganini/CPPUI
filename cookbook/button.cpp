#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void button(Model & model) {
        for(auto column: CPPUI::VBox().layout(CPPUI::layout::weight(1))) {
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label(static_cast<std::string>(model.buttonValue));
                CPPUI::Button("A").click([&model]() { model.buttonValue = std::string("A"); });
                CPPUI::Button("B").click([&model]() { model.buttonValue = std::string("B"); });
                CPPUI::Button("Disabled").disabled().click([&model]() { model.buttonValue = std::string("Disabled"); });
                CPPUI::Button("Styled").style(CPPUI::style::color(0x244B5A), CPPUI::style::bgColor(0xFFF7E6), CPPUI::style::fontSize(20));
                CPPUI::Spacer();
            }
            CPPUI::Spacer();
        }
    }
}

#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void hbox(Model & model) {
        for(auto column: CPPUI::VBox()) {
            CPPUI::Checkbox("Reverse horizontal order",model.inverseH).id("hbox-inverse");
            for(auto row: CPPUI::HBox(model.inverseH).id("spacer-row")) {
                CPPUI::Label("Column 1");
                CPPUI::Divider();
                CPPUI::Spacer().id("row-gap");
                CPPUI::Label("Column 2").id("row-end");
            }
            for(auto row: CPPUI::HBox(model.inverseH)) {
                CPPUI::Label("Column 1");
                CPPUI::Divider();
                CPPUI::Label("Column 2").layout(CPPUI::layout::weight(1));
                CPPUI::Divider();
                CPPUI::Label("Column 3");
            }
            CPPUI::Divider();
            CPPUI::Label("Default gap: 10 pixels");
            for(auto box:CPPUI::HBox(model.inverseH)) {
                CPPUI::Label("First"); CPPUI::Label("Second"); CPPUI::Label("Third");
            }
            CPPUI::Divider();
            CPPUI::Label("gap=12 pixels");
            for(auto box:CPPUI::HBox(model.inverseH,12)) {
                CPPUI::Label("First"); CPPUI::Label("Second"); CPPUI::Label("Third");
            }
        }
        CPPUI::Spacer();
    }
}

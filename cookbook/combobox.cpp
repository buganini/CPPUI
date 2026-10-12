#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void combobox(Model & model) {
        for(auto column: CPPUI::VBox()) {
            CPPUI::Label("Index: " + std::to_string(static_cast<int>(model.comboIndex)));
            CPPUI::Label("Text: " + static_cast<std::string>(model.comboText));
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label("Readonly");
                for(auto combo: CPPUI::ComboBox(false, model.comboText).id("combo-readonly")) {
                    CPPUI::ComboBoxItem("Item 1", "Value 1");
                    CPPUI::ComboBoxItem("Item 2", "Value 2");
                    CPPUI::ComboBoxItem("Item 3", "Value 3");
                }
                CPPUI::Spacer();
            }
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label("Editable");
                for(auto combo: CPPUI::ComboBox(true, model.comboIndex, model.comboText).id("combo-editable")) {
                    CPPUI::ComboBoxItem("Item 1");
                    CPPUI::ComboBoxItem("Item 2");
                    CPPUI::ComboBoxItem("Item 3", "Value 3");
                }
                CPPUI::Spacer();
            }
            CPPUI::Spacer();
        }
    }
}

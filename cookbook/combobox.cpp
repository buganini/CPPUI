#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void combobox(Model & model) {
        for(auto column: Presentation::VBox()) {
            Presentation::Label("Index: " + std::to_string(static_cast<int>(model.comboIndex)));
            Presentation::Label("Text: " + static_cast<std::string>(model.comboText));
            for(auto row: Presentation::HBox()) {
                Presentation::Label("Readonly");
                for(auto combo: CPPUI::ComboBox(false, model.comboText).id("combo-readonly")) {
                    CPPUI::ComboBoxItem("Item 1", "Value 1");
                    CPPUI::ComboBoxItem("Item 2", "Value 2");
                    CPPUI::ComboBoxItem("Item 3", "Value 3");
                }
                Presentation::Spacer();
            }
            for(auto row: Presentation::HBox()) {
                Presentation::Label("Editable");
                for(auto combo: CPPUI::ComboBox(true, model.comboIndex, model.comboText).id("combo-editable")) {
                    CPPUI::ComboBoxItem("Item 1");
                    CPPUI::ComboBoxItem("Item 2");
                    CPPUI::ComboBoxItem("Item 3", "Value 3");
                }
                Presentation::Spacer();
            }
            Presentation::Spacer();
        }
    }
}

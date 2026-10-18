#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void combobox(Model & model) {
        for(auto column: CPPUI::VBox()) {
            CPPUI::Label("Index: " + std::to_string(static_cast<int>(model.comboIndex)));
            CPPUI::Label("Text: " + static_cast<std::string>(model.comboText));
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label("Non-editable");
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
            CPPUI::Divider();
            for(bool editable: {false, true}) for(auto row: CPPUI::HBox()) {
                CPPUI::Label(editable ? "Disabled (editable)" : "Disabled (non-editable)");
                for(auto combo: CPPUI::ComboBox(editable, model.comboDisabledText).disabled().id(editable ? "combo-disabled-editable" : "combo-disabled")) {
                    CPPUI::ComboBoxItem("Item 1", "Value 1");
                    CPPUI::ComboBoxItem("Item 2", "Value 2");
                    CPPUI::ComboBoxItem("Item 3", "Value 3");
                }
                CPPUI::Spacer();
            }
            CPPUI::Divider();
            CPPUI::Label("Autocomplete: type 'Tai' or 'Tok', then choose a suggestion.");
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label("Destination");
                for(auto combo: CPPUI::ComboBox(true, model.destination).id("combo-autocomplete")) {
                    CPPUI::ComboBoxItem("Amsterdam (AMS)", "AMS");
                    CPPUI::ComboBoxItem("London Heathrow (LHR)", "LHR");
                    CPPUI::ComboBoxItem("Paris Charles de Gaulle (CDG)", "CDG");
                    CPPUI::ComboBoxItem("Seoul Incheon (ICN)", "ICN");
                    CPPUI::ComboBoxItem("Singapore Changi (SIN)", "SIN");
                    CPPUI::ComboBoxItem("Taipei Taoyuan (TPE)", "TPE");
                    CPPUI::ComboBoxItem("Taipei Songshan (TSA)", "TSA");
                    CPPUI::ComboBoxItem("Tokyo Haneda (HND)", "HND");
                    CPPUI::ComboBoxItem("Tokyo Narita (NRT)", "NRT");
                }
                CPPUI::Button("Clear autocomplete").click([&model]() { model.destination = std::string(); });
                CPPUI::Spacer();
            }
            const auto destination = static_cast<std::string>(model.destination);
            CPPUI::Label("Autocomplete value: " + (destination.empty() ? "(empty)" : destination));
            CPPUI::Spacer();
        }
    }
}

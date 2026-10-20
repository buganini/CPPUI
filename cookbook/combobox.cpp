#include "cppui/backend.hpp"
#include "recipes.hpp"
#include <sstream>
#include <cctype>

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
                CPPUI::Label("Editable (autoexpand off)");
                for(auto combo: CPPUI::ComboBox(true, model.comboIndex, model.comboText).autoexpand(false).id("combo-editable")) {
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
            CPPUI::Label("Autocomplete: type 'tai tok' to match Taipei or Tokyo.");
            CPPUI::Label("Matching is application-defined.");
            auto lower = [](std::string text) {
                for(auto & c: text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
                return text;
            };
            std::istringstream query(lower(static_cast<std::string>(model.destinationQuery)));
            std::vector<std::string> keywords;
            for(std::string keyword; query >> keyword;) keywords.push_back(keyword);
            for(auto row: CPPUI::HBox()) {
                CPPUI::Label("Destination");
                for(auto combo: CPPUI::ComboBox(true, model.destination).editModel(model.destinationQuery).id("combo-autocomplete")) {
                    const char * destinations[][2] = {
                        {"Amsterdam (AMS)", "AMS"}, {"London Heathrow (LHR)", "LHR"},
                        {"Paris Charles de Gaulle (CDG)", "CDG"}, {"Seoul Incheon (ICN)", "ICN"},
                        {"Singapore Changi (SIN)", "SIN"}, {"Taipei Taoyuan (TPE)", "TPE"},
                        {"Taipei Songshan (TSA)", "TSA"}, {"Tokyo Haneda (HND)", "HND"},
                        {"Tokyo Narita (NRT)", "NRT"}
                    };
                    for(const auto & destination: destinations) {
                        const auto label = lower(destination[0]);
                        if(keywords.empty() || std::any_of(keywords.begin(), keywords.end(), [&](const std::string & keyword) {
                            return label.find(keyword) != std::string::npos;
                        })) CPPUI::ComboBoxItem(destination[0], destination[1]);
                    }
                }
                CPPUI::Spacer();
            }
            CPPUI::Button("Clear autocomplete").click([&model]() {
                model.destination = std::string(); model.destinationQuery = std::string();
            });
            const auto destination = static_cast<std::string>(model.destination);
            CPPUI::Label("Autocomplete value: " + (destination.empty() ? "(empty)" : destination));
            CPPUI::Spacer();
        }
    }
}

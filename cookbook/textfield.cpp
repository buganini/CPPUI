#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void textfield(Model & model) {
        for(auto column: Presentation::VBox()) {
            for(auto row: Presentation::HBox()) {
                CPPUI::Button("Clear").click([&model]() { model.fieldText = std::string(); });
                CPPUI::Button("Set something").click([&model]() { model.fieldText = std::string("something"); });
                Presentation::Spacer();
            }
            Presentation::Label("State Model");
            Presentation::Label("State:" + static_cast<std::string>(model.fieldText));
            CPPUI::TextField(model.fieldText).id("field-model");
            Presentation::Label("Editing Buffer Model");
            Presentation::Label("State:" + static_cast<std::string>(model.fieldText));
            Presentation::Label("Editing:" + static_cast<std::string>(model.fieldEditing));
            CPPUI::TextField(model.fieldText, model.fieldEditing).id("field-editing");
            Presentation::Spacer();
        }
    }
}

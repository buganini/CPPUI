#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void textfield(Model & model) {
        for(auto column: CPPUI::VBox()) {
            for(auto row: CPPUI::HBox()) {
                CPPUI::Button("Clear").click([&model]() { model.fieldText = std::string(); });
                CPPUI::Button("Set something").click([&model]() { model.fieldText = std::string("something"); });
                CPPUI::Spacer();
            }
            CPPUI::Label("State Model");
            CPPUI::Label("State:" + static_cast<std::string>(model.fieldText));
            CPPUI::TextField(model.fieldText).placeholder("Enter text").id("field-model");
            CPPUI::Label("Editing Buffer Model");
            CPPUI::Label("State:" + static_cast<std::string>(model.fieldText));
            CPPUI::Label("Editing:" + static_cast<std::string>(model.fieldEditing));
            CPPUI::TextField(model.fieldText, model.fieldEditing).id("field-editing");
            CPPUI::Spacer();
        }
    }
}

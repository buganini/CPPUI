#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void radiobutton(Model & model) {
        for(auto column: CPPUI::VBox()) {
            const auto value = static_cast<std::string>(model.radioValue);
            CPPUI::Label("Model value: " + (value.empty() ? std::string("None") : value));
            CPPUI::RadioButton("A", "a", model.radioValue).id("radio-a");
            CPPUI::RadioButton("B", "b", model.radioValue).id("radio-b");
            CPPUI::RadioButton("Disabled", "disabled", model.radioValue).disabled().id("radio-disabled");
            CPPUI::Button("Select disabled option").click([&model]() { model.radioValue = std::string("disabled"); });
            CPPUI::Spacer();
        }
    }
}

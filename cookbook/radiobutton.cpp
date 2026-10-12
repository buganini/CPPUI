#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void radiobutton(Model & model) {
        for(auto column: Presentation::VBox()) {
            const auto value = static_cast<std::string>(model.radioValue);
            Presentation::Label("Model value: " + (value.empty() ? std::string("None") : value));
            CPPUI::RadioButton("A", "a", model.radioValue).id("radio-a");
            CPPUI::RadioButton("B", "b", model.radioValue).id("radio-b");
            Presentation::Spacer();
        }
    }
}

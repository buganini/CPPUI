#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void maskedfield(Model & model) {
        for(auto column: CPPUI::VBox()) {
            CPPUI::Label("Password (committed on Enter or blur)");
            CPPUI::MaskedField(model.password).placeholder("Enter a password").id("password");
            CPPUI::Label("Committed length: " + std::to_string(static_cast<std::string>(model.password).size()));
            CPPUI::Label("Password with an editing buffer");
            CPPUI::MaskedField(model.bufferedPassword, model.passwordEditing)
                .placeholder("Enter another password").id("buffered-password");
            CPPUI::Label("Editing length: " + std::to_string(static_cast<std::string>(model.passwordEditing).size()));
            CPPUI::Label("Committed length: " + std::to_string(static_cast<std::string>(model.bufferedPassword).size()));
            CPPUI::Button("Clear passwords").click([&model]() {
                model.password = std::string(); model.bufferedPassword = std::string(); model.passwordEditing = std::string();
            });
            CPPUI::Spacer();
        }
    }
}

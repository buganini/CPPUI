#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void binding(Model & model) {
        // A Binding getter can run during declaration construction. Keep its log
        // in plain storage; publish a State revision after the native commit's
        // setter/getter/change sequence, never mutate State inside the builder.
        (void)static_cast<int>(model.bindingRevision);
        CPPUI::Binding<std::string> value([&model]() {
            model.bindingLog("getter_func");return model.bindingValue;
        },[&model](const std::string & value) {
            model.bindingLog("setter_func "+value);model.bindingValue=value;
        });
        for(auto column:CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Label("Shared binding");
            CPPUI::Label("Selected value: "+std::string(model.bindingChoice));
            for(auto combo:CPPUI::ComboBox(false,model.bindingChoice).id("binding-choice")) {
                CPPUI::ComboBoxItem("Item 1","Value 1");CPPUI::ComboBoxItem("Item 2","Value 2");CPPUI::ComboBoxItem("Item 3","Value 3");
            }
            for(auto value:{"Value 1","Value 2","Value 3"}) CPPUI::RadioButton(value,std::string(value),model.bindingChoice);
            CPPUI::Divider();
            CPPUI::Label("Custom getter / setter");
            CPPUI::Label("var: "+model.bindingValue);
            CPPUI::TextField(value).change([&model]() {
                model.bindingLog("on_changed_func "+model.bindingValue);
                model.bindingRevision+=1;
            }).id("binding-field");
            CPPUI::Label("Binding log (latest 100 messages)");
            for(auto scroll:CPPUI::Scroll().layout({}, 0, 1))
                CPPUI::Text(model.bindingLogText()).id("binding-log");
        }
    }
}

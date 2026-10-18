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

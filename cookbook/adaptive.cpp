#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void adaptive(Model & model) {
        auto & container=CPPUI::VBox();container.id("adaptive-container").layout({}, {},1);
        const auto size=container.size().get();
        for(auto scope:container) {
            CPPUI::Label(size ? "Container size: "+std::to_string(size->width)+" × "+std::to_string(size->height)
                              : "Container size: waiting for layout");
            CPPUI::Label("Resize the window: breakpoint = 600");
            for(auto compact:CPPUI::VBox().id("adaptive-compact").visible(true,{{},600})) {
                CPPUI::Label("Compact layout");
                CPPUI::Label("Name");
                CPPUI::TextField(model.adaptiveName).id("adaptive-compact-input");
                CPPUI::Label("The two layouts share the same input state.");
            }
            for(auto wide:CPPUI::HBox().id("adaptive-wide").visible(true,{600})) {
                CPPUI::Label("Wide layout");
                CPPUI::Label("Name");
                CPPUI::TextField(model.adaptiveName).id("adaptive-wide-input");
                CPPUI::Label("Shared input state");
            }
            CPPUI::Spacer();
        }
    }
}

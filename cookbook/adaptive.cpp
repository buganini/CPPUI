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
            CPPUI::Label("Wide layout/style overrides").id("adaptive-overrides")
                .style(CPPUI::style::color(0x777777),CPPUI::style::fontSize(12))
                .wideStyle(600,CPPUI::style::color(0x228855),CPPUI::style::fontSize(18))
                .layout(CPPUI::layout::padding(2))
                .wideLayout(600,CPPUI::layout::padding(12));
            CPPUI::Label("HVBox: one input, retained while the direction changes");
            for(auto box:CPPUI::HVBox(600).id("adaptive-hvbox")) {
                CPPUI::Label("Name");
                CPPUI::TextField(model.adaptiveName).id("adaptive-retained-input");
                CPPUI::Button("Save");
            }
            CPPUI::Label("VHBox: vertical when the parent is tall enough");
            for(auto box:CPPUI::VHBox(500).id("adaptive-vhbox")) {
                CPPUI::Label("First");
                CPPUI::Label("Second");
                CPPUI::Label("Third");
            }
            CPPUI::Spacer();
        }
    }
}

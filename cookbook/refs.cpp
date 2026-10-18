#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void refs(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Button("Increment after 500 ms").click([&model]() {
                auto post=CPPUI::ref("cookbook.refs.poster").node()->getPoster();
                post([&model]() { model.postedCallbacks=static_cast<int>(model.postedCallbacks)+1; },500);
            }).ref("cookbook.refs.poster");
            CPPUI::Label("Callbacks executed on the UI loop: "+std::to_string(static_cast<int>(model.postedCallbacks)));
            CPPUI::Label("Fixed ref names identify live nodes across updates.");
            for(auto row: CPPUI::HBox()) {
                CPPUI::Button("Select first").click([](){CPPUI::ref("cookbook.refs.tabs").select("first");});
                CPPUI::Button("Select second").click([](){CPPUI::ref("cookbook.refs.tabs").select("second");});
            }
            for(auto tabs: CPPUI::Tabs().ref("cookbook.refs.tabs").layout({}, {}, 1)) {
                for(auto tab: CPPUI::Tab("First").id("first")) CPPUI::Label("Binders follow the current reconciled node.");
                for(auto tab: CPPUI::Tab("Second").id("second")) CPPUI::Label("Window refs provide queued desktop commands.");
            }
            CPPUI::Label("Window controls (desktop backends only). Restore minimized windows using your desktop.");
            const bool desktop=Backend::desktopWindowCommands;
            for(auto row: CPPUI::HBox()) {
                CPPUI::Button("Maximize").disabled(!desktop).click([](){CPPUI::ref("cookbook.window").maximize(true);});
                CPPUI::Button("Unmaximize").disabled(!desktop).click([](){CPPUI::ref("cookbook.window").maximize(false);});
                CPPUI::Button("Minimize").disabled(!desktop).click([](){CPPUI::ref("cookbook.window").minimize(true);});
            }
            for(auto row: CPPUI::HBox()) {
                CPPUI::Button("Fullscreen").disabled(!desktop).click([](){CPPUI::ref("cookbook.window").fullscreen(true);});
                CPPUI::Button("Exit fullscreen").disabled(!desktop).click([](){CPPUI::ref("cookbook.window").fullscreen(false);});
            }
        }
    }
}

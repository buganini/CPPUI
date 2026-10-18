#include "backend.hpp"
#include "../recipes.hpp"

namespace Cookbook {
    namespace {
        class TabExample: public CPPUI::Qt::QtView {
            CPPUI::State<CPPUI::OptionalText> selection{CPPUI::OptionalText("first")};
            void pages() {
                for(auto tab: CPPUI::Tab("Tab 1").id("first")) CPPUI::Label("Content 1");
                for(auto tab: CPPUI::Tab("Tab 2")) CPPUI::Label("Content 2");
            }
            void choose(const std::string & key) {
                CPPUI::ref("cookbook.tabs").select(key);
            }
            void content() override {
                for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
                    CPPUI::Label("Select with ref().select(): internal model");
                    for(auto row: CPPUI::HBox()) {
                        CPPUI::Button("Tab 1").click([this]() { choose("first"); });
                        CPPUI::Button("Tab 2").click([this]() { choose("Tab 2"); });
                    }
                    for(auto tabs: CPPUI::Tabs(CPPUI::Side::TOP).layout({}, {}, 1).id("example-tabs").ref("cookbook.tabs")) pages();
                    const auto key = static_cast<CPPUI::OptionalText>(selection);
                    CPPUI::Label("Select by state: " + (key ? *key : "None"));
                    for(auto row: CPPUI::HBox()) {
                        CPPUI::Button("Tab 1 (state)").click([this]() { selection = CPPUI::OptionalText("first"); });
                        CPPUI::Button("Tab 2 (state)").click([this]() { selection = CPPUI::OptionalText("Tab 2"); });
                    }
                    for(auto tabs: CPPUI::Tabs(CPPUI::Side::BOTTOM, selection).layout({}, {}, 1).id("bound-tabs")) pages();
                }
            }
        };
    }
    void tab(Model &) { CPPUI::SubView<TabExample>(); }
}

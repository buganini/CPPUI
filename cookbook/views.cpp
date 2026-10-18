#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    namespace {
        class Editor: public Backend::View {
            CPPUI::State<std::string> text{std::string("This text survives switching views.")};
            void content() override {
                for(auto column: CPPUI::VBox()) {
                    CPPUI::Label("Editor");
                    CPPUI::TextField(text).id("views-editor-field");
                    CPPUI::Button("Change text").click([this]() { text = std::string("retained draft"); });
                    CPPUI::Label("Saved text: " + static_cast<std::string>(text));
                    CPPUI::Spacer();
                }
            }
        };
        class Example: public Backend::View {
            CPPUI::State<CPPUI::OptionalText> selection{CPPUI::OptionalText("editor")};
            void pages() {
                CPPUI::SubView<Editor>().id("editor");
                for(auto column: CPPUI::VBox().id("settings")) {
                    CPPUI::Label("Settings");
                    CPPUI::Label("Return to Editor to see the retained text.");
                    CPPUI::Spacer();
                }
            }
            void choose(CPPUI::OptionalText key) {
                CPPUI::ref("cookbook.views").select(key);
            }
            void content() override {
                for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
                    CPPUI::Label("Select with ref().select(): internal model, initially empty");
                    for(auto row: CPPUI::HBox()) {
                        CPPUI::Button("Editor").click([this]() { choose(CPPUI::OptionalText("editor")); });
                        CPPUI::Button("Settings").click([this]() { choose(CPPUI::OptionalText("settings")); });
                        CPPUI::Button("Clear").click([this]() { choose({}); });
                    }
                    for(auto views: CPPUI::Views().layout({}, {}, 1).id("internal-views").ref("cookbook.views")) pages();
                    const auto key = static_cast<CPPUI::OptionalText>(selection);
                    CPPUI::Label("Select by state: " + (key ? *key : "None"));
                    for(auto row: CPPUI::HBox()) {
                        CPPUI::Button("Editor (state)").click([this]() { selection = CPPUI::OptionalText("editor"); });
                        CPPUI::Button("Settings (state)").click([this]() { selection = CPPUI::OptionalText("settings"); });
                        CPPUI::Button("Clear (state)").click([this]() { selection = CPPUI::OptionalText(); });
                    }
                    for(auto views: CPPUI::Views(selection).layout({}, {}, 1).id("bound-views")) pages();
                }
            }
        };
    }
    void views(Model &) { CPPUI::SubView<Example>(); }
}

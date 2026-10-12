#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    namespace {
        std::string describe(const CPPUI::OptionalText & text) { return text ? "\"" + *text + "\"" : "None"; }
        std::string describe(const CPPUI::Optional<std::vector<std::string>> & paths) {
            if(!paths) return "None";
            std::string result = "[";
            for(const auto & path: *paths) {
                if(result.size() > 1) result += ", ";
                result += "\"" + path + "\"";
            }
            return result + "]";
        }
    }
    void dialog(Model & model) {
        for(auto column: Presentation::VBox()) {
            Presentation::Label("Directory: " + describe(static_cast<CPPUI::OptionalText>(model.directory)));
            CPPUI::Button("Open Directory").click([&model]() { model.directory = CPPUI::OpenDirectory(); });
            Presentation::Label("File: " + describe(static_cast<CPPUI::OptionalText>(model.file)));
            CPPUI::Button("Open File").click([&model]() { model.file = CPPUI::OpenFile(); });
            CPPUI::Button("Open File with Filter").click([&model]() { model.file = CPPUI::OpenFile("Open File", "", "Text files (*.txt)"); });
            CPPUI::Button("Save File (default to model)").click([&model]() {
                model.file = CPPUI::SaveFile(static_cast<CPPUI::OptionalText>(model.file));
            });
            Presentation::Label("Files: " + describe(static_cast<CPPUI::Optional<std::vector<std::string>>>(model.files)));
            CPPUI::Button("Open Files").click([&model]() { model.files = CPPUI::OpenFiles(); });
            Presentation::Label("Information:");
            CPPUI::Button("Information").click([]() { CPPUI::Information("info message", "info title"); });
            Presentation::Label("Warning:");
            CPPUI::Button("Warning").click([]() { CPPUI::Warning("warning message", "warning title"); });
            Presentation::Label("Critical:");
            CPPUI::Button("Critical").click([]() { CPPUI::Critical("critical message", "critical title"); });
            const auto confirmed = static_cast<CPPUI::Optional<bool>>(model.confirm);
            Presentation::Label(std::string("Confirm: ") + (confirmed ? (*confirmed ? "True" : "False") : "None"));
            CPPUI::Button("Confirm").click([&model]() { model.confirm = CPPUI::Confirm("confirm message", "confirm title"); });
            Presentation::Label("Prompt: " + describe(static_cast<CPPUI::OptionalText>(model.prompt)));
            CPPUI::Button("Prompt").click([&model]() { model.prompt = CPPUI::Prompt("prompt message", "prompt title", "default"); });
            Presentation::Spacer();
        }
    }
}

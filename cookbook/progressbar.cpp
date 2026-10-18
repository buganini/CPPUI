#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void progressbar(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            for(auto row: CPPUI::HBox()) {
                CPPUI::Button("-5").click([&model]() { model.progress -= 5; });
                CPPUI::Label(std::to_string(static_cast<int>(model.progress)));
                CPPUI::Button("+5").click([&model]() { model.progress += 5; });
            }
            CPPUI::ProgressBar(model.progress, 100);
            CPPUI::Spacer();
        }
    }
}

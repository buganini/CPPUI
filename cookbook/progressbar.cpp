#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void progressbar(Model & model) {
        for(auto column: Presentation::VBox(1)) {
            for(auto row: Presentation::HBox()) {
                CPPUI::Button("-5").click([&model]() { model.progress -= 5; });
                CPPUI::Label(std::to_string(static_cast<int>(model.progress)));
                CPPUI::Button("+5").click([&model]() { model.progress += 5; });
            }
            CPPUI::ProgressBar(model.progress, 100);
            Presentation::Spacer();
        }
    }
}

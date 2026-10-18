#include "backend.hpp"
#include "recipes.hpp"
#include <iostream>

using namespace CPPUI;

#if defined(CPPUI_COOKBOOK_QT)
namespace Cookbook {
    void modal(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Label(std::string("State: ") + (static_cast<bool>(model.modalOpen) ? "True" : "False"));
            Button("Open Modal").click([&model]() { model.modalOpen = true; });
            CPPUI::Spacer();
            for(auto window: Modal(model.modalOpen).title("Modal Window")
                    .open([]() { std::cout << "on_open\n"; })
                    .close([]() { std::cout << "on_close\n"; }).id("example-modal")) {
                Button("Close Modal").click([&model]() { model.modalOpen = false; });
            }
        }
    }
}
#endif

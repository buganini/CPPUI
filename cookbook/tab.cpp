#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void tab(Model &) {
        for(auto tabs: CPPUI::Tabs().id("example-tabs")) {
            for(auto first: CPPUI::Tab("Tab 1")) {
                CPPUI::Label("Content 1");
            }
            for(auto second: CPPUI::Tab("Tab 2")) {
                CPPUI::Label("Content 2");
            }
        }
    }
}

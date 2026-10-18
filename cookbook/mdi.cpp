#include "backend.hpp"
#include "recipes.hpp"

#if defined(CPPUI_COOKBOOK_QT)
namespace Cookbook {
    void mdi(Model &) {
        for(auto area: CPPUI::MdiArea()) {
            for(auto first: CPPUI::MdiSubWindow()) for(auto column: CPPUI::VBox()) {
                CPPUI::Label("1");
                CPPUI::Label("2");
            }
            for(auto second: CPPUI::MdiSubWindow()) for(auto column: CPPUI::VBox()) {
                CPPUI::Label("A");
                CPPUI::Label("B");
            }
        }
    }
}
#endif

#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void image(Model &) {
        for(auto column: CPPUI::VBox()) {
            CPPUI::Image(CPPUI_COOKBOOK_ASSET_DIR "/hello_world.png").layout({}, 360);
            CPPUI::Spacer();
        }
    }
}

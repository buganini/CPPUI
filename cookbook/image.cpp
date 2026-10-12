#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void image(Model &) {
        for(auto column: Presentation::VBox()) {
            Presentation::Image(CPPUI_COOKBOOK_ASSET_DIR "/hello_world.png");
            Presentation::Spacer();
        }
    }
}

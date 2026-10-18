#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void image(Model &) {
        for(auto column: CPPUI::VBox()) {
            CPPUI::Image(CPPUI_COOKBOOK_ASSET_DIR "/windmill.jpg").layout(400);
            CPPUI::Label("The Windmill at Wijk bij Duurstede");
            CPPUI::Label("Jacob van Ruisdael, c. 1668–1670");
            CPPUI::Label("Rijksmuseum, Amsterdam · Public Domain");
            CPPUI::Label("https://id.rijksmuseum.nl/200107959");
            CPPUI::Spacer();
        }
    }
}

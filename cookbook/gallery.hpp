#ifndef CPPUI_COOKBOOK_GALLERY
#define CPPUI_COOKBOOK_GALLERY
#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    inline const std::string & title() {
        static const std::string value = std::string("Cookbook (") + CPPUI::Backend::name + ")";
        return value;
    }
    class Gallery: public CPPUI::Application {
        public:
        CPPUI::State<int> selected{0};
        Model model;
        Gallery(int argc, char ** argv): Application(argc, argv) { title(Cookbook::title()); }
        void content() override;
    };
}
#endif

#include "backend.hpp"
#include "recipes.hpp"
#include <iostream>
#include <stdexcept>

namespace Cookbook {
    class Gallery: public CPPUI::Application {
        public:
        CPPUI::State<int> selected{0};
        Model model;
        using Application::Application;

        void content() override {
            const auto & page = recipes().at(static_cast<int>(selected));
            for(auto window: CPPUI::Window().id("cookbook")) {
                for(auto column: CPPUI::VBox()) {
                    CPPUI::Label(std::string("CPPUI cookbook / ") + Backend::name);
                    for(auto navigation: CPPUI::HBox()) {
                        CPPUI::Button("Previous").click([this]() { move(-1); });
                        CPPUI::Label(page.title);
                        CPPUI::Button("Next").click([this]() { move(1); });
                    }
                    CPPUI::Label(std::string(page.render ? "Remaining: " : "Unavailable: ") + page.remaining);
                    for(auto body: CPPUI::VBox().id(std::string("recipe:") + page.key)) {
                        if(page.render) page.render(model);
                    }
                }
            }
        }
        void move(int delta) {
            const int size = static_cast<int>(recipes().size());
            selected = (static_cast<int>(selected) + delta + size) % size;
        }
    };

    bool click(CPPUI::Node & node, const std::string & text) {
        if(Backend::clickButton(node, text)) return true;
        for(auto child: *node.children) {
            if(click(*child, text)) return true;
        }
        return false;
    }
    bool hasText(CPPUI::Node & node, const std::string & text) {
        if(Backend::labelHasText(node, text)) return true;
        for(auto child: *node.children) {
            if(hasText(*child, text)) return true;
        }
        return false;
    }
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }

    int run(int argc, char ** argv) {
        try {
            bool smoke = false;
            for(int i = 1; i < argc; ++i) {
                if(std::string(argv[i]) == "--list") {
                    for(const auto & page: recipes()) {
                        std::cout << page.key << " [" << (page.render ? "partial" : "unavailable")
                                  << "]: " << page.remaining << '\n';
                    }
                    return 0;
                }
                if(std::string(argv[i]) == "--smoke") smoke = true;
            }
            Gallery gallery(argc, argv);
            if(!smoke) {
                gallery.run();
                return 0;
            }
            gallery.redraw();
            // Revisit pages to exercise replacement and retired callback cleanup.
            for(int cycle = 0; cycle < 3; ++cycle) {
                for(std::size_t i = 0; i < recipes().size(); ++i) {
                    gallery.selected = static_cast<int>(i);
                    Backend::settle();
                    require(hasText(gallery, recipes()[i].title), "page selection did not render");
                    if(std::string(recipes()[i].key) == "button") {
                        require(click(gallery, "B"), "button recipe is missing B");
                        Backend::settle();
                        require(hasText(gallery, "B"), "B did not update the label");
                        require(click(gallery, "A"), "button recipe is missing A");
                        Backend::settle();
                        require(hasText(gallery, "A"), "A did not update the label");
                    }
                }
            }
            require(click(gallery, "Next"), "gallery navigation is missing");
            Backend::settle();
            require(static_cast<int>(gallery.selected) == 0, "next did not wrap to the first page");
            require(click(gallery, "Previous"), "gallery navigation is missing");
            Backend::settle();
            require(static_cast<int>(gallery.selected) == static_cast<int>(recipes().size()) - 1,
                    "previous did not wrap to the last page");
            std::cout << Backend::name << " cookbook smoke passed\n";
            return 0;
        } catch(const std::exception & error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}

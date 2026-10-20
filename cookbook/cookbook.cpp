#include "gallery.hpp"
#include <iostream>
#include <stdexcept>

namespace Cookbook {
    void menus() {
#if CPPUI_HAS_MENUS
        for(auto bar: CPPUI::MenuBar()) {
            for(auto menu: CPPUI::Menu("Menu")) {
                CPPUI::MenuAction("Action").trigger([]() { std::cout << "action\n"; });
                CPPUI::MenuAction("Action2").trigger([]() { std::cout << "action2\n"; });
                for(auto submenu: CPPUI::Menu("Menu2")) {
                    CPPUI::MenuAction("Action3").trigger([]() { std::cout << "action3\n"; });
                    CPPUI::MenuAction("Action4").trigger([]() { std::cout << "action4\n"; });
                }
            }
            for(auto menu: CPPUI::Menu("Menu3")) {
                CPPUI::MenuAction("Action5");
                CPPUI::MenuAction("Action6");
            }
        }
#endif
    }

    void Gallery::content() {
        const auto & page = recipes().at(static_cast<int>(selected));
        auto & host = CPPUI::Window().title(Cookbook::title()).size(1024,768);
        host.ref("cookbook.window").id("cookbook");
        for(auto window: host) {
            menus();
            for(auto row: CPPUI::HBox().id("gallery")) {
                for(auto menu: CPPUI::VBox().layout(200).id("menu-container")) {
                    CPPUI::Label(Cookbook::title());
                    for(auto scroll: CPPUI::Scroll().layout(200, 0, 1).id("menu-scroller")) {
                        for(auto list: CPPUI::VBox().id("menu-list")) {
                            for(std::size_t i = 0; i < recipes().size(); ++i) {
                                auto & link = CPPUI::Label(recipes()[i].title);
                                link.id(std::string("page:") + recipes()[i].key);
                                link.click([this, i]() { selected = static_cast<int>(i); });
                            }
                            CPPUI::Spacer();
                        }
                    }
                }
                for(auto result: CPPUI::VBox().layout({}, {}, 1).id("result")) {
                    CPPUI::Label("Result");
                    for(auto body: CPPUI::VBox().layout({}, {}, 1).id(std::string("recipe:") + page.key)) {
                        if(page.render) page.render(model);
                        else {
                            CPPUI::Label("Not yet ported to this backend");
                            CPPUI::Spacer();
                        }
                    }
                }
            }
        }
    }

    int run(int argc, char ** argv) {
        try {
            for(int i = 1; i < argc; ++i) {
                const std::string argument = argv[i];
                if(argument == "--list") {
                    for(const auto & page: recipes())
                        std::cout << page.key << " [" << (page.render ? (*page.remaining ? "partial" : "ported") : "not ported")
                                  << "]: " << page.remaining << '\n';
                    return 0;
                }
                if(argument == "--smoke" || argument == "--heavy-smoke")
                    throw std::invalid_argument("Run the separate _smoke test executable for cookbook checks");
            }
            Gallery gallery(argc, argv);
            gallery.run();
            return 0;
        } catch(const std::exception & error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}

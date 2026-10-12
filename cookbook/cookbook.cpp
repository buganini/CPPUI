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
            for(auto window: Presentation::Window().id("cookbook")) {
                Presentation::Menus();
                for(auto row: Presentation::HBox().id("gallery")) {
                    for(auto menu: Presentation::Sidebar().id("menu-container")) {
                        Presentation::Label("Example");
                        for(auto scroll: Presentation::Scroll().id("menu-scroller")) {
                            for(auto list: Presentation::VBox().id("menu-list")) {
                                for(std::size_t i = 0; i < recipes().size(); ++i) {
                                    auto & link = Presentation::PageLink(recipes()[i].title);
                                    link.id(std::string("page:") + recipes()[i].key);
                                    link.click([this, i]() { selected = static_cast<int>(i); });
                                }
                                Presentation::Spacer();
                            }
                        }
                    }
                    for(auto result: Presentation::VBox(1).id("result")) {
                        Presentation::Label("Result");
                        for(auto body: Presentation::VBox(1).id(std::string("recipe:") + page.key)) {
                            if(page.render) page.render(model);
                            else {
                                Presentation::Label("Not Supported for this backend");
                                Presentation::Spacer();
                            }
                        }
                    }
                }
            }
        }
    };

    CPPUI::Node * find(CPPUI::Node & node, const std::string & id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) {
            if(auto found = find(*child, id)) return found;
        }
        return nullptr;
    }
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
                        std::cout << page.key << " [" << (page.render ? (*page.remaining ? "partial" : "ported") : "unavailable")
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
            Backend::mount(gallery);
            Backend::settle();
            Backend::prepareLayoutCheck(gallery);
            auto sidebar = find(gallery, "menu-scroller")->outer();
            Backend::checkLayout(gallery);
            // Revisit pages to exercise replacement and retired callback cleanup.
            for(int cycle = 0; cycle < 3; ++cycle) {
                for(std::size_t i = 0; i < recipes().size(); ++i) {
                    auto link = find(gallery, std::string("page:") + recipes()[i].key);
                    require(link && Backend::selectPage(*link), "sidebar page link is missing");
                    Backend::settle();
                    require(static_cast<int>(gallery.selected) == static_cast<int>(i), "sidebar did not select the page");
                    require(find(gallery, std::string("recipe:") + recipes()[i].key), "page selection did not render");
                    require(find(gallery, "menu-scroller")->outer() == sidebar, "page selection replaced the sidebar");
                    require(Backend::pageVisible(gallery, recipes()[i].key), "selected sidebar link is clipped or lost its scroll position");
                    Backend::checkLayout(gallery);
                    if(std::string(recipes()[i].key) == "text" && recipes()[i].render) {
                        require(hasText(gallery, "Multiple\nLines\nText"), "text recipe did not render");
                    }
                    if(std::string(recipes()[i].key) == "splitter" && recipes()[i].render) {
                        require(hasText(gallery, "item 1") && hasText(gallery, "item 4")
                            && hasText(gallery, "pane") && hasText(gallery, "blah blah"),
                            "splitter recipe did not render its panes");
                    }
                    if(std::string(recipes()[i].key) == "canvas" && recipes()[i].render) {
                        const int previousAngle = gallery.model.rotation;
                        require(click(gallery, "+"), "canvas recipe is missing its rotation button");
                        Backend::settle();
                        require(static_cast<int>(gallery.model.rotation) == previousAngle + 15
                            && hasText(gallery, "Rotate: " + std::to_string(previousAngle + 15)),
                            "canvas rotation did not update");
                        Backend::checkLayout(gallery);
                    }
                    if(std::string(recipes()[i].key) == "tab" && recipes()[i].render) {
                        auto tabs = find(gallery, "example-tabs");
                        require(tabs && hasText(*tabs, "Content 1") && hasText(*tabs, "Content 2"),
                            "tab recipe did not render its contents");
                        require(Backend::selectTab(*tabs, 1, "Tab 2") && Backend::selectTab(*tabs, 0, "Tab 1"),
                            "tab recipe could not switch pages");
                    }
                    if(std::string(recipes()[i].key) == "checkbox" && recipes()[i].render) {
                        for(const auto & id: {"checkbox-1", "checkbox-3", "checkbox-6"}) {
                            auto checkbox = find(gallery, id);
                            require(checkbox && Backend::toggleCheckbox(*checkbox), "checkbox recipe is missing a control");
                            Backend::settle();
                        }
                        require(!static_cast<bool>(gallery.model.checkboxOne)
                            && static_cast<std::vector<std::string>>(gallery.model.checkboxList) == std::vector<std::string>{"value 3"}
                            && static_cast<std::map<std::string, bool>>(gallery.model.checkboxMap) == std::map<std::string, bool>{{"value 6", true}},
                            "checkbox boolean/list/map changes did not update models");
                        require(hasText(gallery, "Model value 1: False"), "bound checkbox did not redraw labels");
                        for(const auto & id: {"checkbox-1", "checkbox-3", "checkbox-6"}) {
                            require(Backend::toggleCheckbox(*find(gallery, id)), "checkbox toggle failed after redraw");
                            Backend::settle();
                        }
                        require(static_cast<bool>(gallery.model.checkboxOne)
                            && static_cast<std::vector<std::string>>(gallery.model.checkboxList).empty()
                            && static_cast<std::map<std::string, bool>>(gallery.model.checkboxMap).empty(),
                            "checkbox collection removal did not clear membership");
                    }
                    if(std::string(recipes()[i].key) == "dialog" && recipes()[i].render) {
                        Backend::answerPrompt("", true);
                        require(click(gallery, "Prompt"), "dialog recipe is missing Prompt");
                        Backend::settle();
                        require(hasText(gallery, "Prompt: \"\""), "accepted empty prompt was lost");
                        Backend::answerPrompt("ignored", false);
                        require(click(gallery, "Prompt"), "dialog recipe is missing Prompt after redraw");
                        Backend::settle();
                        require(hasText(gallery, "Prompt: None"), "cancelled prompt remained a value");
                    }
                    if(std::string(recipes()[i].key) == "modal" && recipes()[i].render) {
                        require(click(gallery, "Open Modal"), "modal recipe is missing Open Modal");
                        Backend::settle();
                        auto modal = find(gallery, "example-modal");
                        require(modal && Backend::modalVisible(*modal) && hasText(gallery, "State: True"),
                            "modal recipe did not open and synchronize its state");
                        require(click(gallery, "Close Modal"), "modal recipe is missing Close Modal");
                        Backend::settle();
                        modal = find(gallery, "example-modal");
                        require(modal && !Backend::modalVisible(*modal) && hasText(gallery, "State: False"),
                            "modal recipe did not close and synchronize its state");
                    }
                    if(std::string(recipes()[i].key) == "progressbar" && recipes()[i].render) {
                        require(click(gallery, "-5"), "progress recipe is missing -5");
                        Backend::settle();
                        require(hasText(gallery, "45"), "progress did not decrease");
                        require(click(gallery, "+5"), "progress recipe is missing +5");
                        Backend::settle();
                        require(hasText(gallery, "50"), "progress did not increase");
                    }
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
            require(!click(gallery, "Next") && !click(gallery, "Previous"), "obsolete navigation remains");
            std::cout << Backend::name << " cookbook smoke passed\n";
            return 0;
        } catch(const std::exception & error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}

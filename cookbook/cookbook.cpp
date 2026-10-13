#include "backend.hpp"
#include "recipes.hpp"
#include <iostream>
#include <cstdlib>
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
                Backend::configureWindow(window);
                Backend::menus();
                for(auto row: CPPUI::HBox().id("gallery")) {
                    for(auto menu: CPPUI::VBox().layout(200).id("menu-container")) {
                        CPPUI::Label("Example");
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
                        std::cout << page.key << " [" << (page.render ? (*page.remaining ? "partial" : "ported") : "not ported")
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
                    if(std::getenv("CPPUI_COOKBOOK_TRACE")) std::cerr << "Checking " << recipes()[i].key << " (cycle " << cycle + 1 << ")\n";
                    auto link = find(gallery, std::string("page:") + recipes()[i].key);
                    require(link && Backend::selectPage(*link), "sidebar page link is missing");
                    Backend::settle();
                    require(static_cast<int>(gallery.selected) == static_cast<int>(i), "sidebar did not select the page");
                    require(find(gallery, std::string("recipe:") + recipes()[i].key), "page selection did not render");
                    require(find(gallery, "menu-scroller")->outer() == sidebar, "page selection replaced the sidebar");
                    require(Backend::pageVisible(gallery, recipes()[i].key), "selected sidebar link is clipped or lost its scroll position");
                    Backend::checkLayout(gallery);
                    if(std::string(recipes()[i].key) == "views" && recipes()[i].render) {
                        require(Backend::viewSelected(*find(gallery, "internal-views"), "")
                            && Backend::viewSelected(*find(gallery, "bound-views"), "editor"), "Views initial selection is wrong");
                        for(const auto & suffix: {std::string(), std::string(" (state)")}) {
                            const auto id = suffix.empty() ? "internal-views" : "bound-views";
                            require(click(gallery, "Editor" + suffix), "Views Editor button is missing");
                            Backend::settle();
                            auto field = find(*find(gallery, id), "views-editor-field");
                            auto native = field->outer();
                            require(Backend::editTextField(*field, "retained draft", true), "Views editor is missing");
                            Backend::settle();
                            require(click(gallery, "Settings" + suffix), "Views Settings button is missing");
                            Backend::settle();
                            require(Backend::viewSelected(*find(gallery, id), "settings"), "Views did not show Settings");
                            require(click(gallery, "Editor" + suffix), "Views Editor button was lost");
                            Backend::settle();
                            require(Backend::viewSelected(*find(gallery, id), "editor")
                                && find(*find(gallery, id), "views-editor-field")->outer() == native, "Views replaced the hidden editor");
                            require(click(gallery, "Clear" + suffix), "Views Clear button is missing");
                            Backend::settle();
                            require(Backend::viewSelected(*find(gallery, id), ""), "Views did not clear its selection");
                        }
                    }
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
                    if(std::string(recipes()[i].key) == "tree_adapter" && recipes()[i].render) {
                        auto first = find(gallery, "adapter-tree-first");
                        auto second = find(gallery, "adapter-tree-second");
                        const auto before = gallery.model.adapterTree->root->children.size();
                        require(first && second && Backend::treeChildCount(*first, {}) == static_cast<int>(before)
                            && Backend::treeChildCount(*second, {0, 1}) == 2, "adapter tree hierarchy was lost");
                        require(!Backend::treeExpanded(*first, {0}) && Backend::treeExpanded(*second, {0, 1}),
                            "adapter tree initial expansion differed from the recipe");
                        require(click(gallery, "Add item"), "tree adapter recipe is missing Add item");
                        Backend::settle();
                        first = find(gallery, "adapter-tree-first"); second = find(gallery, "adapter-tree-second");
                        require(Backend::treeChildCount(*first, {}) == static_cast<int>(before + 1)
                            && Backend::treeChildCount(*second, {static_cast<int>(before)}) == 1
                            && Backend::treeExpanded(*second, {static_cast<int>(before)}), "tree adapter views did not share an added subtree");
                    }
                    if(std::string(recipes()[i].key) == "tree" && recipes()[i].render) {
                        const int before = gallery.model.treeItems;
                        auto tree = find(gallery, "example-tree");
                        require(tree && Backend::treeChildCount(*tree, {0, 3}) == before
                            && Backend::treeExpanded(*tree, {0}) && Backend::treeExpanded(*tree, {0, 0}), "tree hierarchy or initial expansion was lost");
                        require(click(gallery, "Add item"), "tree recipe is missing Add item");
                        Backend::settle(); tree = find(gallery, "example-tree");
                        require(gallery.model.treeItems == before + 1 && Backend::treeChildCount(*tree, {0, 3}) == before + 1
                            && Backend::treeExpanded(*tree, {0, 3}), "tree growth lost items or expansion");
                    }
                    if(std::string(recipes()[i].key) == "table_adapter" && recipes()[i].render) {
                        auto first = find(gallery, "adapter-table-first");
                        auto second = find(gallery, "adapter-table-second");
                        require(first && second && Backend::editTable(*first, 0, 0, "42"), "table adapter did not accept an editable cell");
                        Backend::settle();
                        first = find(gallery, "adapter-table-first"); second = find(gallery, "adapter-table-second");
                        require(Backend::tableText(*first, 0, 0) == "42" && Backend::tableText(*second, 0, 0) == "42",
                            "table adapter views did not share their edited matrix");
                        require(!Backend::editTable(*second, 3, 0, "99") && Backend::tableText(*second, 3, 0) == "3",
                            "table adapter accepted an edit in its readonly rows");
                    }
                    if(std::string(recipes()[i].key) == "table" && recipes()[i].render) {
                        require(Backend::editTable(*find(gallery, "example-table"), 0, 0, "edited"), "table string cell did not commit");
                        Backend::settle();
                        require(hasText(gallery, "A1: edited"), "table edit did not update the bound label");
                        require(Backend::editTable(*find(gallery, "example-table"), 0, 3, "8"), "table numeric choice did not commit");
                        Backend::settle();
                        require(gallery.model.tableSides == 8 && hasText(gallery, "Sides: 8"), "table choice lost its integer value");
                        require(Backend::editTable(*find(gallery, "example-table"), 1, 3, "CCW"), "table mapped choice did not commit");
                        Backend::settle(); require(hasText(gallery, "Direction: CCW"), "table mapped choice lost its raw value");
                    }
                    if(std::string(recipes()[i].key) == "scroll" && recipes()[i].render) {
                        require(Backend::atScrollEnd(*find(gallery, "scroller")), "scroll recipe did not start at the end");
                        require(click(gallery, "+"), "scroll recipe is missing +");
                        Backend::settle();
                        require(Backend::atScrollEnd(*find(gallery, "scroller")), "scroll recipe lost end anchoring after growth");
                        require(click(gallery, "-"), "scroll recipe is missing -");
                        Backend::settle();
                    }
                    if(std::string(recipes()[i].key) == "combobox" && recipes()[i].render) {
                        require(Backend::selectCombo(*find(gallery, "combo-readonly"), 1), "readonly combo missing");
                        Backend::settle(); require(hasText(gallery, "Text: Value 2"), "combo label/value binding failed");
                        require(Backend::selectCombo(*find(gallery, "combo-editable"), 2), "editable combo missing");
                        Backend::settle(); require(hasText(gallery, "Index: 2"), "editable combo did not commit its index");
                    }
                    if(std::string(recipes()[i].key) == "radiobutton" && recipes()[i].render) {
                        require(Backend::selectRadio(*find(gallery, "radio-a")), "radio A missing");
                        Backend::settle(); require(hasText(gallery, "Model value: a"), "radio A did not commit");
                        require(Backend::selectRadio(*find(gallery, "radio-b")), "radio B missing");
                        Backend::settle(); require(hasText(gallery, "Model value: b"), "radio B did not commit");
                    }
                    if(std::string(recipes()[i].key) == "textfield" && recipes()[i].render) {
                        require(click(gallery, "Clear"), "textfield recipe is missing Clear");
                        Backend::settle();
                        require(Backend::editTextField(*find(gallery, "field-editing"), "draft", false), "textfield draft input failed");
                        Backend::settle();
                        require(static_cast<std::string>(gallery.model.fieldText).empty()
                            && static_cast<std::string>(gallery.model.fieldEditing) == "draft", "typing did not preserve separate model/draft");
                        require(Backend::editTextField(*find(gallery, "field-editing"), "committed", true), "textfield commit failed");
                        Backend::settle();
                        require(hasText(gallery, "State:committed"), "textfield commit did not redraw model labels");
                        require(click(gallery, "Set something"), "textfield recipe is missing Set something");
                        Backend::settle();
                        require(hasText(gallery, "State:something"), "textfield programmatic update failed");
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

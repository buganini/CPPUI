#include "cookbook_backend.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <thread>
namespace Cookbook {
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) {
            if(auto found = find(*child, id)) return found;
        }
        return nullptr;
    }
    bool click(CPPUI::Node & node, const std::string & text) {
        if(TestBackend::clickButton(node, text)) return true;
        for(auto child: *node.children) {
            if(click(*child, text)) return true;
        }
        return false;
    }
    bool hasText(CPPUI::Node & node, const std::string & text) {
        if(TestBackend::labelHasText(node, text)) return true;
        for(auto child: *node.children) {
            if(hasText(*child, text)) return true;
        }
        return false;
    }
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }

    int runSmoke(int argc, char ** argv) {
        try {
            bool heavySmoke = false;
            for(int i = 1; i < argc; ++i) {
                if(std::string(argv[i]) == "--list") {
                    for(const auto & page: recipes()) {
                        std::cout << page.key << " [" << (page.render ? (*page.remaining ? "partial" : "ported") : "not ported")
                                  << "]: " << page.remaining << '\n';
                    }
                    return 0;
                }
                if(std::string(argv[i]) == "--heavy-smoke") heavySmoke = true;
            }
            Gallery gallery(argc, argv);
            gallery.sync();
            TestBackend::settle();
            TestBackend::prepareLayoutCheck(gallery);
            if(heavySmoke) {
                require(TestBackend::selectPage(*find(gallery, "page:heavycanvas")), "HeavyCanvas page missing");
                TestBackend::settle();
                auto samples = find(gallery, "heavy-samples");
                require(samples && samples->children->size() == 14, "large native presets missing");
                require(TestBackend::selectCombo(*samples, 13), "million-session preset missing");
                TestBackend::settle();
                require(hasText(gallery, "Visible: 1000000 / 1000000 sessions | Zoom 1.00x"), "million-session range failed");
                require(find(gallery, "heavy-canvas"), "million-session chart missing");
                require(TestBackend::selectCombo(*find(gallery, "heavy-samples"), 0), "preset reset failed");
                TestBackend::settle();
                require(hasText(gallery, "Visible: 1000 / 1000 sessions | Zoom 1.00x"), "large chart retained stale range");
                std::cout << TestBackend::name << " million-session HeavyCanvas smoke passed\n";
                return 0;
            }
            auto sidebar = find(gallery, "menu-scroller")->outer();
            auto container = find(gallery,"gallery")->outer();
            auto windowSize=find(gallery,"cookbook")->size();
            for(int width:{1200,800,1200,800}) {
                TestBackend::resizeWindow(gallery,width,600);
                for(int pass=0;pass<4;++pass) TestBackend::settle();
                const auto size=windowSize.get();
                require(size && (size->width>=1000)==(width>=1000),"gallery did not retain the requested width");
                TestBackend::checkLayout(gallery);
                require(find(gallery,"gallery")->outer()==container && find(gallery,"menu-scroller")->outer()==sidebar,
                        "gallery resize replaced a native container");
            }
            // Revisit pages to exercise replacement and retired callback cleanup.
            for(int cycle = 0; cycle < 3; ++cycle) {
                for(std::size_t i = 0; i < recipes().size(); ++i) {
                    if(std::getenv("CPPUI_COOKBOOK_TRACE")) std::cerr << "Checking " << recipes()[i].key << " (cycle " << cycle + 1 << ")\n";
                    auto link = find(gallery, std::string("page:") + recipes()[i].key);
                    require(link && TestBackend::selectPage(*link), "sidebar page link is missing");
                    TestBackend::settle();
                    require(static_cast<int>(gallery.selected) == static_cast<int>(i), "sidebar did not select the page");
                    require(find(gallery, std::string("recipe:") + recipes()[i].key), "page selection did not render");
                    require(find(gallery, "menu-scroller")->outer() == sidebar, "page selection replaced the sidebar");
                    require(TestBackend::pageVisible(gallery, recipes()[i].key), "selected sidebar link is clipped or lost its scroll position");
                    TestBackend::checkLayout(gallery);
                    if(std::string(recipes()[i].key) == "adaptive") {
                        for(int pass=0;pass<4;++pass) TestBackend::settle();
                        auto size=find(gallery,"adaptive-container")->size().get();
                        require(bool(size),"adaptive container size was not measured");
                        const bool wide=size->width>=600;
                        auto overrides=find(gallery,"adaptive-overrides");
                        require(overrides && *overrides->style_color==(wide ? 0x1C2A30 : 0xFFF7E6) &&
                                *overrides->style_fontsize==(wide ? 18 : 12) &&
                                overrides->layout_padding->left==(wide ? 12 : 2),"adaptive recipe wide overrides differ from the measured width");
                        require(find(gallery,"adaptive-wide")->is_visible==wide &&
                                find(gallery,"adaptive-compact")->is_visible!=wide,"adaptive cookbook branch differs from measured size");
                        auto field=find(gallery,wide ? "adaptive-wide-input" : "adaptive-compact-input");
                        require(field && TestBackend::editTextField(*field,"Grace",true),"adaptive shared input missing");
                        TestBackend::settle();
                        require(static_cast<std::string>(gallery.model.adaptiveName)=="Grace","adaptive input did not share model state");
                        auto retained=find(gallery,"adaptive-retained-input");
                        require(find(gallery,"adaptive-hvbox") && find(gallery,"adaptive-vhbox") && retained &&
                                TestBackend::editTextField(*retained,"Ada",true),"adaptive box input missing");
                        TestBackend::settle();
                        require(static_cast<std::string>(gallery.model.adaptiveName)=="Ada","adaptive box input did not share model state");
                        auto input=find(gallery,"adaptive-retained-input")->outer();
                        for(const char * id:{"adaptive-inverse-h","adaptive-inverse-v"}) {
                            require(TestBackend::toggleCheckbox(*find(gallery,id)),"adaptive reversal control missing");
                            TestBackend::settle();
                            require(find(gallery,"adaptive-retained-input")->outer()==input,"adaptive reversal replaced the editor");
                        }
                        gallery.model.adaptiveInverseH=false;gallery.model.adaptiveInverseV=false;TestBackend::settle();
                    }
                    if(std::string(recipes()[i].key) == "adaptive_grid") {
                        auto editor=find(gallery,"grid-editor")->outer();
                        TestBackend::resizeWindow(gallery,1250,800);
                        for(int pass=0;pass<4;++pass) TestBackend::settle();
                        require(find(gallery,"adaptive-grid")->grid_columns==4 && find(gallery,"grid-editor")->grid_columnspan==2,
                            "adaptive grid did not reach four columns with a two-column editor");
                        const std::vector<std::pair<std::string,int>> modes={{"ordered",3},{"dense",2},{"forward",4}};
                        for(const auto & mode:modes) {
                            gallery.model.gridMode=mode.first;TestBackend::settle();
                            require(find(gallery,"F")->grid_cell->row==mode.second,"adaptive grid backfill mode differs from PUI");
                            require(find(gallery,"grid-editor")->outer()==editor,"mode change replaced grid editor");
                        }
                        gallery.model.gridShowC=false;TestBackend::settle();
                        require(!find(gallery,"C")->grid_cell,"hidden grid tile still has a cell");
                        require(click(gallery,"Reverse tiles"),"grid reverse button missing");TestBackend::settle();
                        require(find(gallery,"grid-editor")->outer()==editor,"tile reorder replaced editor");
                        require(TestBackend::editTextField(*find(gallery,"grid-editor"),"retained grid input",true),"grid editor missing");
                        TestBackend::settle();
                        require(static_cast<std::string>(gallery.model.gridText)=="retained grid input","grid editor did not write model");
                        gallery.model.gridMode=std::string("ordered");gallery.model.gridShowC=true;gallery.model.gridReverse=false;
                        TestBackend::resizeWindow(gallery,800,600);TestBackend::settle();
                        require(find(gallery,"adaptive-grid")->grid_columns<4 && find(gallery,"grid-editor")->outer()==editor,"narrow grid did not retain editor");
                        auto gridSize=find(gallery,"adaptive-grid")->size().get();
                        require(gridSize && find(gallery,"grid-editor")->grid_columnspan==(gridSize->width>=576 ? 2 : 1),
                            "adaptive editor span did not follow Grid content width");
                    }
                    if(std::string(recipes()[i].key) == "refs") {
                        const int before=gallery.model.postedCallbacks;
                        const auto start=std::chrono::steady_clock::now();
                        require(click(gallery,"Increment after 500 ms"),"Delayed poster button missing");
                        require(static_cast<int>(gallery.model.postedCallbacks)==before,"Delayed cookbook callback ran synchronously");
                        while(static_cast<int>(gallery.model.postedCallbacks)==before &&
                              std::chrono::steady_clock::now()-start<std::chrono::seconds(3)) {
                            TestBackend::settle();std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        }
                        require(static_cast<int>(gallery.model.postedCallbacks)==before+1 &&
                                std::chrono::steady_clock::now()-start>=std::chrono::milliseconds(500),
                                "Delayed cookbook callback failed or fired early");
                        require(click(gallery,"Select second"),"Refs selection button missing"); TestBackend::settle();
                        require(click(gallery,"Select first"),"Refs first button missing"); TestBackend::settle();
                    }
                    if(std::string(recipes()[i].key) == "views" && recipes()[i].render) {
                        require(TestBackend::viewSelected(*find(gallery, "internal-views"), "")
                            && TestBackend::viewSelected(*find(gallery, "bound-views"), "editor"), "Views initial selection is wrong");
                        for(const auto & suffix: {std::string(), std::string(" (state)")}) {
                            const auto id = suffix.empty() ? "internal-views" : "bound-views";
                            require(click(gallery, "Editor" + suffix), "Views Editor button is missing");
                            TestBackend::settle();
                            auto field = find(*find(gallery, id), "views-editor-field");
                            auto native = field->outer();
                            require(click(*find(gallery, id), "Change text"), "Views state control is missing");
                            TestBackend::settle();
                            require(TestBackend::editTextField(*find(*find(gallery, id), "views-editor-field"), "typed draft", true), "Views text input is missing");
                            TestBackend::settle();
                            require(click(gallery, "Settings" + suffix), "Views Settings button is missing");
                            TestBackend::settle();
                            require(TestBackend::viewSelected(*find(gallery, id), "settings"), "Views did not show Settings");
                            require(click(gallery, "Editor" + suffix), "Views Editor button was lost");
                            TestBackend::settle();
                            require(TestBackend::viewSelected(*find(gallery, id), "editor")
                                && find(*find(gallery, id), "views-editor-field")->outer() == native
                                && hasText(*find(gallery, id), "Saved text: typed draft"), "Views replaced the hidden editor or lost its state");
                            require(click(gallery, "Clear" + suffix), "Views Clear button is missing");
                            TestBackend::settle();
                            require(TestBackend::viewSelected(*find(gallery, id), ""), "Views did not clear its selection");
                        }
                    }
                    if(std::string(recipes()[i].key) == "text" && recipes()[i].render) {
                        require(hasText(gallery, "Multiple\nLines\nText"), "text recipe did not render");
                    }
                    if(std::string(recipes()[i].key) == "splitter" && recipes()[i].render) {
                        require(hasText(gallery, "item 1") && hasText(gallery, "item 4")
                            && hasText(gallery, "pane 2") && hasText(gallery, "pane 3") && hasText(gallery, "blah blah"),
                            "splitter recipe did not render its panes");
                    }
                    if(std::string(recipes()[i].key) == "canvas" && recipes()[i].render) {
                        const int previousAngle = gallery.model.rotation;
                        require(click(gallery, "+"), "canvas recipe is missing its rotation button");
                        TestBackend::settle();
                        require(static_cast<int>(gallery.model.rotation) == previousAngle + 15
                            && hasText(gallery, "Rotate: " + std::to_string(previousAngle + 15)),
                            "canvas rotation did not update");
                        TestBackend::checkLayout(gallery);
                    }
                    if(std::string(recipes()[i].key) == "tab" && recipes()[i].render) {
                        auto tabs = find(gallery, "example-tabs");
                        require(tabs && hasText(*tabs, "Content 1") && hasText(*tabs, "Content 2"),
                            "tab recipe did not render its contents");
                        require(TestBackend::selectTab(*tabs, 1, "Tab 2"), "tab recipe could not select its second page");
                        TestBackend::settle();
                        require(TestBackend::selectTab(*find(gallery, "example-tabs"), 0, "Tab 1"), "tab recipe could not select its first page");
                        TestBackend::settle();
                        if(find(gallery, "bound-tabs")) {
                            require(click(gallery, "Tab 2") && click(gallery, "Tab 2 (state)"), "tab selection buttons are missing");
                            TestBackend::settle();
                            require(hasText(gallery, "Select by state: Tab 2"), "State did not select the second tab");
                            require(TestBackend::selectTab(*find(gallery, "bound-tabs"), 0, "Tab 1"), "bound tabs could not select their first page");
                            TestBackend::settle();
                            require(hasText(gallery, "Select by state: first"), "native tab selection did not update State");
                        }
                    }
                    if(std::string(recipes()[i].key) == "tree_adapter" && recipes()[i].render) {
                        auto first = find(gallery, "adapter-tree-first");
                        auto second = find(gallery, "adapter-tree-second");
                        const auto before = gallery.model.adapterTree->root->children.size();
                        require(first && second && TestBackend::treeChildCount(*first, {}) == static_cast<int>(before)
                            && TestBackend::treeChildCount(*second, {0, 1}) == 2, "adapter tree hierarchy was lost");
                        require(!TestBackend::treeExpanded(*first, {0}) && TestBackend::treeExpanded(*second, {0, 1}),
                            "adapter tree initial expansion differed from the recipe");
                        require(click(gallery, "Add item"), "tree adapter recipe is missing Add item");
                        TestBackend::settle();
                        first = find(gallery, "adapter-tree-first"); second = find(gallery, "adapter-tree-second");
                        require(TestBackend::treeChildCount(*first, {}) == static_cast<int>(before + 1)
                            && TestBackend::treeChildCount(*second, {static_cast<int>(before)}) == 1
                            && TestBackend::treeExpanded(*second, {static_cast<int>(before)}), "tree adapter views did not share an added subtree");
                    }
                    if(std::string(recipes()[i].key) == "tree" && recipes()[i].render) {
                        const int before = gallery.model.treeItems;
                        auto tree = find(gallery, "example-tree");
                        require(tree && TestBackend::treeChildCount(*tree, {0, 3}) == before
                            && TestBackend::treeExpanded(*tree, {0}) && TestBackend::treeExpanded(*tree, {0, 0}), "tree hierarchy or initial expansion was lost");
                        require(click(gallery, "Add item"), "tree recipe is missing Add item");
                        TestBackend::settle(); tree = find(gallery, "example-tree");
                        require(gallery.model.treeItems == before + 1 && TestBackend::treeChildCount(*tree, {0, 3}) == before + 1
                            && TestBackend::treeExpanded(*tree, {0, 3}), "tree growth lost items or expansion");
                    }
                    if(std::string(recipes()[i].key) == "binding" && recipes()[i].render) {
                        auto field=find(gallery,"binding-field");
                        require(field && TestBackend::editTextField(*field,"bound",true),"Binding field did not accept an edit");
                        TestBackend::settle();
                        auto log=find(gallery,"binding-log");const auto text=gallery.model.bindingLogText();
                        require(gallery.model.bindingValue=="bound" && hasText(gallery,"var: bound") && log
                            && TestBackend::labelHasText(*log,text),"Binding model or visible log did not update");
                        require(text.find("getter_func")!=std::string::npos && text.find("setter_func bound")!=std::string::npos
                            && text.find("on_changed_func bound")!=std::string::npos,"Binding callbacks missing from in-app log");
                        for(int entry=0;entry<105;++entry) gallery.model.bindingLog("message "+std::to_string(entry));
                        gallery.model.bindingRevision+=1;TestBackend::settle();
                        require(gallery.model.bindingLogs.size()==100
                            && TestBackend::labelHasText(*find(gallery,"binding-log"),gallery.model.bindingLogText()),"Binding log exceeded its cap or failed to render retained entries");
                    }
                    if(std::string(recipes()[i].key) == "table_adapter" && recipes()[i].render) {
                        auto first = find(gallery, "adapter-table-first");
                        auto second = find(gallery, "adapter-table-second");
                        require(first && second && TestBackend::editTable(*first, 0, 0, "42"), "table adapter did not accept an editable cell");
                        TestBackend::settle();
                        first = find(gallery, "adapter-table-first"); second = find(gallery, "adapter-table-second");
                        require(TestBackend::tableText(*first, 0, 0) == "42" && TestBackend::tableText(*second, 0, 0) == "42",
                            "table adapter views did not share their edited matrix");
                        require(!TestBackend::editTable(*second, 3, 0, "99") && TestBackend::tableText(*second, 3, 0) == "3",
                            "table adapter accepted an edit in its readonly rows");
                    }
                    if(std::string(recipes()[i].key) == "table" && recipes()[i].render) {
                        auto table=find(gallery,"example-table");
                        for(auto position:std::vector<std::pair<int,int>>{{0,0},{0,1},{0,3},{1,0},{1,3},{2,0}})
                            require(TestBackend::tableText(*table,position.first,position.second).find("editable: ")==0,"editable table cell missing its marker");
                        require(TestBackend::tableText(*table,0,2)=="A3","readonly table cell marked editable");
                        require(TestBackend::editTable(*find(gallery, "example-table"), 0, 0, "edited"), "table string cell did not commit");
                        TestBackend::settle();
                        require(hasText(gallery, "A1: edited"), "table edit did not update the bound label");
                        require(TestBackend::editTable(*find(gallery, "example-table"), 0, 3, "8"), "table numeric choice did not commit");
                        TestBackend::settle();
                        require(gallery.model.tableSides == 8 && hasText(gallery, "Sides: 8"), "table choice lost its integer value");
                        require(TestBackend::editTable(*find(gallery, "example-table"), 1, 3, "CCW"), "table mapped choice did not commit");
                        TestBackend::settle(); require(hasText(gallery, "Direction: CCW"), "table mapped choice lost its raw value");
                        require(TestBackend::editTable(*find(gallery, "example-table"), 2, 0, "12"), "decorated price cell did not commit");
                        TestBackend::settle(); require(hasText(gallery, "Price: $12 USD")
                            && TestBackend::tableText(*find(gallery, "example-table"), 2, 0)=="editable: Price: $12 USD", "table price decoration altered the raw model");
                    }
                    if(std::string(recipes()[i].key) == "scroll" && recipes()[i].render) {
                        require(TestBackend::atScrollEnd(*find(gallery, "scroller")), "scroll recipe did not start at the end");
                        require(click(gallery, "+"), "scroll recipe is missing +");
                        TestBackend::settle();
                        require(TestBackend::atScrollEnd(*find(gallery, "scroller")), "scroll recipe lost end anchoring after growth");
                        require(click(gallery, "-"), "scroll recipe is missing -");
                        TestBackend::settle();
                    }
                    if(std::string(recipes()[i].key) == "combobox" && recipes()[i].render) {
                        require(TestBackend::selectCombo(*find(gallery, "combo-readonly"), 1), "readonly combo missing");
                        TestBackend::settle(); require(hasText(gallery, "Text: Value 2"), "combo label/value binding failed");
                        require(TestBackend::selectCombo(*find(gallery, "combo-editable"), 2), "editable combo missing");
                        TestBackend::settle(); require(hasText(gallery, "Index: 2"), "editable combo did not commit its index");
                        require(TestBackend::selectCombo(*find(gallery, "combo-autocomplete"), 5), "autocomplete combo missing");
                        TestBackend::settle(); require(hasText(gallery, "Autocomplete value: TPE"), "autocomplete stored its label instead of the code");
                        require(click(gallery, "Clear autocomplete"), "autocomplete clear missing");
                        TestBackend::settle(); require(hasText(gallery, "Autocomplete value: (empty)"), "autocomplete clear failed");
                        require(TestBackend::typeCombo(*find(gallery, "combo-autocomplete"), "  TAI\t tok  "), "autocomplete editor missing");
                        TestBackend::settle();
                        auto candidates = find(gallery, "combo-autocomplete");
                        require(candidates->children->size() == 4, "application whitespace/OR matching did not retain four candidates");
                        require(TestBackend::selectCombo(*candidates, 2), "filtered autocomplete choice missing");
                        TestBackend::settle(); require(hasText(gallery, "Autocomplete value: HND"), "filtered candidate used wrong index/value");
                        require(click(gallery, "Clear autocomplete"), "autocomplete clear missing after selection");
                        TestBackend::settle();
                        require(find(gallery, "combo-autocomplete")->children->size() == 9 && hasText(gallery, "Autocomplete value: (empty)"),
                            "clear did not reset both query and selected value");
                    }
                    if(std::string(recipes()[i].key) == "heavycanvas" && recipes()[i].render) {
                        // The chart deliberately disables zoom below 320x360.
                        // Leave enough space after the responsive menu and controls.
                        TestBackend::resizeWindow(gallery,1200,800);
                        for(int pass=0;pass<4;++pass) TestBackend::settle();
                        require(find(gallery, "heavy-canvas"), "HeavyCanvas is missing its chart");
                        require(TestBackend::selectCombo(*find(gallery, "heavy-samples"), 0), "HeavyCanvas sample selector missing");
                        TestBackend::settle();
                        require(hasText(gallery, "Visible: 1000 / 1000 sessions | Zoom 1.00x"), "HeavyCanvas initial range differs");
                        require(TestBackend::wheelCanvas(*find(gallery, "heavy-canvas"), 120), "HeavyCanvas wheel input missing");
                        TestBackend::settle(); require(hasText(gallery, "Visible: 800 / 1000 sessions | Zoom 1.25x"), "HeavyCanvas did not zoom in");
                        TestBackend::wheelCanvas(*find(gallery, "heavy-canvas"), -120);
                        TestBackend::settle(); require(hasText(gallery, "Visible: 1000 / 1000 sessions | Zoom 1.00x"), "HeavyCanvas did not zoom out");
                        TestBackend::wheelCanvas(*find(gallery, "heavy-canvas"), 120); TestBackend::settle();
                        require(click(gallery, "Reset zoom"), "HeavyCanvas reset missing");
                        TestBackend::settle(); require(hasText(gallery, "Visible: 1000 / 1000 sessions | Zoom 1.00x"), "HeavyCanvas reset did not restore the range");
                        TestBackend::wheelCanvas(*find(gallery, "heavy-canvas"), 120); TestBackend::settle();
                        require(TestBackend::selectCombo(*find(gallery, "heavy-samples"), 8), "HeavyCanvas 30000-session preset missing");
                        TestBackend::settle(); require(hasText(gallery, "Visible: 30000 / 30000 sessions | Zoom 1.00x"), "HeavyCanvas 30000-session preset failed");
                        TestBackend::selectCombo(*find(gallery, "heavy-samples"), 0); TestBackend::settle();
                        require(hasText(gallery, "Visible: 1000 / 1000 sessions | Zoom 1.00x"), "HeavyCanvas restored obsolete zoom after changing presets");
                        require(TestBackend::selectCombo(*find(gallery, "heavy-samples"), 1), "HeavyCanvas sample selector missing");
                        TestBackend::settle(); require(hasText(gallery, "Visible: 3000 / 3000 sessions | Zoom 1.00x"), "HeavyCanvas sample count did not update");
                        require(click(gallery, "Regenerate") && click(gallery, "Reset zoom"), "HeavyCanvas controls missing");
                        TestBackend::settle();
                        TestBackend::resizeWindow(gallery,800,600);
                        for(int pass=0;pass<4;++pass) TestBackend::settle();
                    }
                    if(std::string(recipes()[i].key) == "maskedfield" && recipes()[i].render) {
                        require(TestBackend::editTextField(*find(gallery, "buffered-password"), "secret", false), "masked input missing");
                        TestBackend::settle();
                        require(static_cast<std::string>(gallery.model.bufferedPassword).empty()
                            && static_cast<std::string>(gallery.model.passwordEditing)=="secret", "masked draft committed too early");
                        require(TestBackend::editTextField(*find(gallery, "buffered-password"), "secret", true), "masked commit failed");
                        TestBackend::settle(); require(hasText(gallery, "Committed length: 6"), "masked model did not commit");
                        require(click(gallery, "Clear passwords"), "masked clear missing"); TestBackend::settle();
                        require(static_cast<std::string>(gallery.model.bufferedPassword).empty(), "masked clear failed");
                    }
                    if(std::string(recipes()[i].key) == "radiobutton" && recipes()[i].render) {
                        require(TestBackend::selectRadio(*find(gallery, "radio-a")), "radio A missing");
                        TestBackend::settle(); require(hasText(gallery, "Model value: a"), "radio A did not commit");
                        require(TestBackend::selectRadio(*find(gallery, "radio-b")), "radio B missing");
                        TestBackend::settle(); require(hasText(gallery, "Model value: b"), "radio B did not commit");
                        require(find(gallery, "radio-disabled") && click(gallery, "Select disabled option"), "disabled radio example missing");
                        TestBackend::settle(); require(hasText(gallery, "Model value: disabled"), "disabled radio model update failed");
                    }
                    if(std::string(recipes()[i].key) == "textfield" && recipes()[i].render) {
                        require(click(gallery, "Clear"), "textfield recipe is missing Clear");
                        TestBackend::settle();
                        require(TestBackend::editTextField(*find(gallery, "field-editing"), "draft", false), "textfield draft input failed");
                        TestBackend::settle();
                        require(static_cast<std::string>(gallery.model.fieldText).empty()
                            && static_cast<std::string>(gallery.model.fieldEditing) == "draft", "typing did not preserve separate model/draft");
                        require(TestBackend::editTextField(*find(gallery, "field-editing"), "committed", true), "textfield commit failed");
                        TestBackend::settle();
                        require(hasText(gallery, "State:committed"), "textfield commit did not redraw model labels");
                        require(click(gallery, "Set something"), "textfield recipe is missing Set something");
                        TestBackend::settle();
                        require(hasText(gallery, "State:something"), "textfield programmatic update failed");
                    }
                    if(std::string(recipes()[i].key) == "checkbox" && recipes()[i].render) {
                        for(const auto & id: {"checkbox-1", "checkbox-3", "checkbox-6"}) {
                            auto checkbox = find(gallery, id);
                            require(checkbox && TestBackend::toggleCheckbox(*checkbox), "checkbox recipe is missing a control");
                            TestBackend::settle();
                        }
                        require(!static_cast<bool>(gallery.model.checkboxOne)
                            && static_cast<std::vector<std::string>>(gallery.model.checkboxList) == std::vector<std::string>{"value 3"}
                            && static_cast<std::map<std::string, bool>>(gallery.model.checkboxMap) == std::map<std::string, bool>{{"value 6", true}},
                            "checkbox boolean/list/map changes did not update models");
                        require(hasText(gallery, "Model value 1: False"), "bound checkbox did not redraw labels");
                        for(const auto & id: {"checkbox-1", "checkbox-3", "checkbox-6"}) {
                            require(TestBackend::toggleCheckbox(*find(gallery, id)), "checkbox toggle failed after redraw");
                            TestBackend::settle();
                        }
                        require(static_cast<bool>(gallery.model.checkboxOne)
                            && static_cast<std::vector<std::string>>(gallery.model.checkboxList).empty()
                            && static_cast<std::map<std::string, bool>>(gallery.model.checkboxMap).empty(),
                            "checkbox collection removal did not clear membership");
                    }
                    if(std::string(recipes()[i].key) == "dialog" && recipes()[i].render) {
                        TestBackend::answerPrompt("", true);
                        require(click(gallery, "Prompt"), "dialog recipe is missing Prompt");
                        TestBackend::settle();
                        require(hasText(gallery, "Prompt: \"\""), "accepted empty prompt was lost");
                        TestBackend::answerPrompt("ignored", false);
                        require(click(gallery, "Prompt"), "dialog recipe is missing Prompt after redraw");
                        TestBackend::settle();
                        require(hasText(gallery, "Prompt: None"), "cancelled prompt remained a value");
                    }
                    if(std::string(recipes()[i].key) == "modal" && recipes()[i].render) {
                        require(click(gallery, "Open Modal"), "modal recipe is missing Open Modal");
                        TestBackend::settle();
                        auto modal = find(gallery, "example-modal");
                        require(modal && TestBackend::modalVisible(*modal) && hasText(gallery, "State: True"),
                            "modal recipe did not open and synchronize its state");
                        require(click(gallery, "Close Modal"), "modal recipe is missing Close Modal");
                        TestBackend::settle();
                        modal = find(gallery, "example-modal");
                        require(modal && !TestBackend::modalVisible(*modal) && hasText(gallery, "State: False"),
                            "modal recipe did not close and synchronize its state");
                    }
                    if(std::string(recipes()[i].key) == "progressbar" && recipes()[i].render) {
                        require(click(gallery, "-5"), "progress recipe is missing -5");
                        TestBackend::settle();
                        require(hasText(gallery, "45"), "progress did not decrease");
                        require(click(gallery, "+5"), "progress recipe is missing +5");
                        TestBackend::settle();
                        require(hasText(gallery, "50"), "progress did not increase");
                    }
                    if(std::string(recipes()[i].key) == "button") {
                        require(click(gallery, "B"), "button recipe is missing B");
                        TestBackend::settle();
                        require(hasText(gallery, "B"), "B did not update the label");
                        require(click(gallery, "A"), "button recipe is missing A");
                        TestBackend::settle();
                        require(hasText(gallery, "A"), "A did not update the label");
                    }
                }
            }
            require(!click(gallery, "Next") && !click(gallery, "Previous"), "obsolete navigation remains");
            std::cout << TestBackend::name << " cookbook smoke passed\n";
            return 0;
        } catch(const std::exception & error) {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
int main(int argc, char ** argv) { return Cookbook::runSmoke(argc, argv); }

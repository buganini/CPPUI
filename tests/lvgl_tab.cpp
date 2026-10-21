#include "cppui/lvgl/tab.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/state.hpp"
#include <map>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    int declarations = 0;
    void count(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
    class TrackedTab: public CPPUI::LVGL::Tab {
        public:
        explicit TrackedTab(const std::string & label): Tab(label) { ++declarations; }
        ~TrackedTab() override { --declarations; }
    };
    class Pair: public CPPUI::LVGL::View {
        public:
        void content() override { CPPUI::Label("pair 1"); CPPUI::Label("pair 2"); }
    };
    enum Kind { Single, VirtualPair, Empty, Grouped };
    struct Page {
        std::string key, title;
        Kind kind;
        Page(const char * key, const char * title, Kind kind = Single): key(key), title(title), kind(kind) {}
    };
    class Root: public CPPUI::LVGL::View {
        public:
        bool show = true;
        std::function<void()> action;
        CPPUI::Side position = CPPUI::Side::Top;
        std::vector<Page> pages{{"a", "A"}, {"b", "B"}, {"c", "C"}};
        void content() override {
            for(auto window: CPPUI::Window()) if(show) for(auto tabs: CPPUI::Tabs(position)) {
                for(const auto & page: pages) for(auto tab: (new TrackedTab(page.title))->id(page.key)) {
                    switch(page.kind) {
                        case Single: CPPUI::Label(page.key).click([this]() { auto invoke = action; if(invoke) invoke(); }); break;
                        case VirtualPair: CPPUI::SubView<Pair>(); break;
                        case Empty: break;
                        case Grouped:
                            for(auto box: CPPUI::VBox()) { CPPUI::Label("group 1"); CPPUI::Label("group 2"); }
                            break;
                    }
                }
            }
        }
        CPPUI::Node * tabs() { return children->front()->children->front(); }
        lv_obj_t * native() { return static_cast<lv_obj_t *>(tabs()->ui); }
        lv_obj_t * widget(int index) { return lv_obj_get_child(lv_obj_get_child(lv_tabview_get_content(native()), index), 0); }
        lv_obj_t * selected() { return widget(lv_tabview_get_tab_active(native())); }
        std::string title(int index) { return lv_label_get_text(lv_obj_get_child(lv_tabview_get_tab_button(native(), index), 0)); }
        void check() {
            require(lv_tabview_get_tab_count(native()) == pages.size(), "incorrect single-child page count");
            for(unsigned i = 0; i < pages.size(); ++i) require(pages[i].key == lv_label_get_text(widget(i)) && pages[i].title == title(i),
                                                            "page order or updated title is wrong");
            require(declarations == static_cast<int>(pages.size()), "old Tab declarations survived redraw");
        }
    };
    void ordinaryTabs() {
        int removed = 0, deleted = 0;
        {
            Root root;
            root.redraw();
            root.check();
            auto tabs = root.native();
            lv_obj_add_event_cb(tabs, count, LV_EVENT_DELETE, &deleted);
            auto tab = root.tabs()->children->front();
            require(tab->is_virtual && !tab->ui, "Tab acquired a native object");
            std::map<std::string, lv_obj_t *> retained;
            for(unsigned i = 0; i < root.pages.size(); ++i) retained[root.pages[i].key] = root.widget(i);
            lv_obj_send_event(lv_tabview_get_tab_button(tabs, 1), LV_EVENT_CLICKED, nullptr);
            require(root.selected() == retained["b"], "native tab click did not select its page");
            for(int i = 0; i < 10; ++i) {
                root.redraw();
                root.check();
                require(root.native() == tabs && root.selected() == retained["b"], "unchanged redraw lost native identity or selection");
            }
            root.pages[1].title = "Renamed";
            root.redraw();
            root.check();
            require(root.selected() == retained["b"], "label change lost selection");
            root.pages.insert(root.pages.begin(), {"new", "New"});
            root.redraw();
            root.check();
            require(root.selected() == retained["b"], "insertion before selected page lost selection");
            for(int i = 0; i < 10; ++i) {
                root.pages = {{"c", "C"}, {"a", "A"}, {"b", "B"}};
                root.redraw();
                root.check();
                require(root.widget(0) == retained["c"] && root.widget(1) == retained["a"] && root.widget(2) == retained["b"], "keyed reorder remounted native contents");
                require(root.selected() == retained["b"], "keyed reorder lost selection");
                root.pages = {{"b", "B"}, {"c", "C"}, {"a", "A"}};
                root.redraw();
                root.check();
                require(root.selected() == retained["b"], "moving selected page lost selection");
            }
            lv_obj_add_event_cb(retained["b"], count, LV_EVENT_DELETE, &removed);
            root.pages = {{"c", "C"}, {"a", "A"}};
            root.redraw();
            root.check();
            require(removed == 1 && lv_tabview_get_tab_active(tabs) < 2, "removal leaked page or left invalid selection");
            root.pages.clear();
            root.redraw();
            root.check();
            require(lv_obj_get_child_count(lv_tabview_get_content(tabs)) == 0 && lv_obj_get_child_count(tabs) == 2,
                    "empty Tabs retained wrappers or staged content");
            root.pages = {{"a", "Again"}};
            root.position = CPPUI::Side::Bottom;
            root.redraw();
            root.check();
            for(const auto & placement: std::map<CPPUI::Side, lv_dir_t>{
                    {CPPUI::Side::Top, LV_DIR_TOP}, {CPPUI::Side::Right, LV_DIR_RIGHT},
                    {CPPUI::Side::Bottom, LV_DIR_BOTTOM}, {CPPUI::Side::Left, LV_DIR_LEFT}}) {
                root.position = placement.first; root.redraw();
                require(root.native() == tabs && lv_tabview_get_tab_bar_position(tabs) == placement.second,
                        "tab position change lost native identity or placement");
            }
            root.show = false;
            root.redraw();
            require(declarations == 0 && deleted == 1, "Tabs removal leaked declarations/native container");
            root.show = true;
            root.redraw();
        }
        require(declarations == 0 && lv_obj_get_child_count(lv_screen_active()) == 0, "root teardown leaked tabs");
    }
    void callbackMovesPage() {
        Root root;
        root.redraw();
        lv_tabview_set_active(root.native(), 1, LV_ANIM_OFF);
        auto child = root.selected();
        root.action = [&root, child]() {
            root.pages = {{"b", "B"}, {"c", "C"}, {"a", "A"}};
            root.redraw();
            require(root.selected() == child, "callback move lost native selection");
            root.show = false;
            root.redraw();
        };
        lv_obj_send_event(child, LV_EVENT_CLICKED, nullptr);
        require(declarations == 0 && lv_obj_get_child_count(lv_screen_active()) == 0,
                "callback move followed by removal leaked tabs");
    }
    void virtualPages() {
        int removed = 0;
        {
            Root root;
            root.pages = {{"multi", "Multi", VirtualPair}, {"last", "Last"}};
            root.redraw();
            auto tabs = root.native(), first = root.widget(0), second = root.widget(1), last = root.widget(2);
            require(lv_tabview_get_tab_count(tabs) == 3 && std::string(lv_label_get_text(first)) == "pair 1"
                    && std::string(lv_label_get_text(second)) == "pair 2", "virtual children were grouped or discarded");
            require(root.title(0) == "Multi" && root.title(1) == "Last" && root.title(2) == "Last", "multi-child labels differ from PUI");
            lv_tabview_set_active(tabs, 1, LV_ANIM_OFF);
            for(int i = 0; i < 10; ++i) {
                root.pages = {{"last", "Last"}, {"multi", "Multi", VirtualPair}};
                root.redraw();
                require(root.widget(0) == last && root.widget(1) == first && root.widget(2) == second && root.selected() == second,
                        "virtual move lost order, identity or selection");
                require(root.title(0) == "Last" && root.title(1) == "Multi" && root.title(2) == "Multi", "moved children lost ancestor label");
                root.pages = {{"multi", "Multi", VirtualPair}, {"last", "Last"}};
                root.redraw();
                require(root.widget(0) == first && root.widget(1) == second && root.widget(2) == last && root.selected() == second,
                        "reverse virtual move lost order, identity or selection");
            }
            lv_obj_add_event_cb(first, count, LV_EVENT_DELETE, &removed);
            lv_obj_add_event_cb(second, count, LV_EVENT_DELETE, &removed);
            root.pages = {{"empty", "Empty", Empty}, {"last", "Last"}};
            root.redraw();
            require(removed == 2 && lv_tabview_get_tab_count(tabs) == 1 && root.widget(0) == last, "empty Tab created a page or virtual removal leaked");
            require(root.title(0) == "Empty", "empty Tab label quirk differs from PUI");
            root.pages = {{"group", "Group", Grouped}};
            root.redraw();
            require(lv_tabview_get_tab_count(tabs) == 1 && lv_obj_get_child_count(root.widget(0)) == 2, "explicit VBox did not group controls");
            auto declaration = root.tabs();
            lv_obj_delete(tabs);
            require(!declaration->ui, "native deletion left stale Tabs observer");
        }
        require(declarations == 0, "virtual Tab teardown leaked declarations");
    }
    class ChangingPages: public CPPUI::LVGL::View {
        CPPUI::State<int> & order;
        public:
        explicit ChangingPages(CPPUI::State<int> & order): order(order) {}
        void content() override {
            if(order == 0) return;
            if(order == 1) { CPPUI::Label("one").id("one"); CPPUI::Label("two").id("two"); }
            else { CPPUI::Label("two").id("two"); CPPUI::Label("one").id("one"); }
        }
    };
    class Independent: public CPPUI::LVGL::View {
        public:
        CPPUI::State<int> order{1};
        void content() override {
            for(auto window: CPPUI::Window()) for(auto tabs: CPPUI::Tabs()) for(auto tab: CPPUI::Tab("Changing")) new ChangingPages(order);
        }
        lv_obj_t * native() { return static_cast<lv_obj_t *>(children->front()->children->front()->ui); }
    };
    void independentView() {
        Independent root;
        root.redraw();
        auto tabs = root.native();
        lv_tabview_set_active(tabs, 1, LV_ANIM_OFF);
        for(int i = 0; i < 10; ++i) {
            root.order = 2;
            require(lv_tabview_get_tab_active(tabs) == 0, "independent keyed move lost selection");
            root.order = 1;
            require(lv_tabview_get_tab_active(tabs) == 1, "independent reverse move lost selection");
        }
        root.order = 0;
        require(lv_tabview_get_tab_count(tabs) == 0, "independent empty view kept pages");
        root.order = 1;
        require(lv_tabview_get_tab_count(tabs) == 2, "independent view did not repopulate");
    }
}
int main() {
    lv_init();
    lv_display_create(800, 600);
    int result = 0;
    try { ordinaryTabs(); virtualPages(); independentView(); callbackMovesPage(); }
    catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

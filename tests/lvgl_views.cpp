#include "cppui/lvgl/views.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { lv_obj_update_layout(lv_screen_active()); }
    int selected(lv_obj_t * book) {
        for(uint32_t i = 0; i < lv_obj_get_child_count(book); ++i)
            if(!lv_obj_is_hidden(lv_obj_get_child(book, i))) return static_cast<int>(i) + 1;
        return 0;
    }
    lv_obj_t * selectedPage(lv_obj_t * book) {
        const int index = selected(book); return index ? lv_obj_get_child(book, index - 1) : nullptr;
    }
    void deleted(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
    int editors = 0;
    class Editor: public CPPUI::LVGL::View {
        public:
        std::string name;
        CPPUI::State<std::string> text{std::string("original")};
        int local = 0;
        explicit Editor(const std::string & name): name(name) { ++editors; }
        ~Editor() override { --editors; }
        void content() override {
            if(name == "empty") return;
            CPPUI::Label(name);
            CPPUI::Label(static_cast<std::string>(text));
        }
        lv_obj_t * field() { return static_cast<lv_obj_t *>(children->at(1)->outer()); }
    };
    class Root: public CPPUI::LVGL::View {
        public:
        std::vector<std::string> keys{"a", "b", "empty"};
        CPPUI::State<CPPUI::OptionalText> first{CPPUI::OptionalText("a")}, second{CPPUI::OptionalText("b")};
        bool show = true, external = false, other = false, missingID = false;
        int writes = 0;
        std::function<void()> action;
        void content() override {
            for(auto window: CPPUI::Window()) if(show) {
                auto state = other ? &second : &first;
                CPPUI::Binding<CPPUI::OptionalText> binding([state]() { return static_cast<CPPUI::OptionalText>(*state); },
                    [this, state](const CPPUI::OptionalText & key) {
                        ++writes; auto invoke = action; *state = key; if(invoke) invoke();
                    });
                auto & container = external ? CPPUI::Views(binding) : CPPUI::Views();
                for(auto scope: container) for(const auto & key: keys) {
                    auto editor = new Editor(key);
                    if(!missingID) editor->id(key);
                }
            }
        }
        CPPUI::LVGL::Views * views() { return static_cast<CPPUI::LVGL::Views *>(children->front()->children->front()); }
        lv_obj_t * native() { return static_cast<lv_obj_t *>(views()->ui); }
        Editor * editor(int index) { return static_cast<Editor *>(views()->children->at(index)->children->front()); }
    };
}
int main() {
    lv_init();
    auto display = lv_display_create(800, 600);
    int result = 0;
    try {
        {
            Root root; root.redraw();
            auto stack = root.native();
            require(lv_obj_get_child_count(stack) == 3 && selected(stack) == 0 && editors == 3, "internal model did not start empty or pages were not grouped");
            require(lv_obj_get_child_count(lv_obj_get_child(stack, 0)) == 2
                && lv_obj_get_child_count(lv_obj_get_child(stack, 2)) == 0, "multi-root or empty view was not retained as one page");
            auto editor = root.editor(0);
            int removed = 0;
            lv_obj_add_event_cb(lv_obj_get_child(stack, 1), deleted, LV_EVENT_DELETE, &removed);
            auto field = editor->field(); editor->local = 42;
            editor->text = std::string("draft"); settle();
            root.views()->select("a"); settle();
            auto page = selectedPage(stack);
            require(lv_obj_get_width(page) == lv_obj_get_content_width(stack)
                && lv_obj_get_height(page) == lv_obj_get_content_height(stack), "selected page did not fill its container");
            require(selected(stack) == 1, "internal selection did not update");
            root.views()->select("b"); settle();
            require(!lv_obj_is_visible(page) && lv_obj_is_visible(lv_obj_get_child(stack, 1)), "selection did not hide/show native pages");
            editor->text = std::string("hidden draft"); settle();
            require(selected(stack) == 2 && editor->field() == field && std::string(lv_label_get_text(field)) == "hidden draft",
                "hidden view could not redraw independently");
            editor->text = std::string("draft"); settle();
            root.views()->select("a"); settle();
            root.keys = {"empty", "a"}; root.redraw(); settle();
            require(root.native() == stack && selectedPage(stack) == page && selected(stack) == 2
                && root.editor(1) == editor && editor->local == 42 && editor->field() == field && std::string(lv_label_get_text(field)) == "draft"
                && removed == 1 && editors == 2, "switch/reorder/removal lost retained view state or leaked pages");
            editor->redraw(); settle();
            require(std::string(lv_label_get_text(field)) == "draft", "independent hidden-view redraw lost the editor buffer");
            root.views()->select(); settle();
            require(selected(stack) == 0, "clearing internal selection did not show the empty page");
            root.views()->select("unknown"); settle();
            require(selected(stack) == 0, "unknown selection did not show the empty page");
            root.keys.push_back("unknown"); root.redraw();
            require(selected(stack) == 3, "returning selected key was not restored");
            root.keys.clear(); root.redraw(); settle();
            require(lv_obj_get_child_count(stack) == 0 && editors == 0, "empty Views retained declared pages");
            root.keys = {"a"}; root.redraw();
            for(const auto & invalid: std::vector<std::vector<std::string>>{{""}, {"a", "a"}}) {
                root.keys = invalid;
                bool rejected = false;
                try { root.redraw(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && root.native() == stack && lv_obj_get_child_count(stack) == 1 && editors == 1
                    && CPPUI::getStack()->empty() && root.frames.empty(), "invalid page IDs corrupted mounted UI or construction scopes");
            }
            root.keys = {"a"}; root.missingID = true;
            bool rejected = false;
            try { root.redraw(); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && editors == 1 && lv_obj_get_child_count(stack) == 1, "missing page ID was accepted or leaked declarations");
            root.missingID = false; root.redraw();
            rejected = false;
            try { root.views()->select(""); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && selected(stack) == 0, "invalid imperative selection damaged the container");
        }
        settle(); require(editors == 0, "internal Views teardown retained views");
        {
            Root root; root.external = true; root.redraw();
            auto stack = root.native();
            require(selected(stack) == 1 && root.writes == 0, "bound selection was not read silently");
            root.first = CPPUI::OptionalText("b"); settle();
            require(selected(stack) == 2 && root.writes == 0, "state selection caused feedback");
            root.views()->select("a"); settle();
            require(*static_cast<CPPUI::OptionalText>(root.first) == "a" && root.writes == 1, "select did not write the bound model once");
            root.other = true; root.redraw(); root.views()->select(); settle();
            require(!static_cast<CPPUI::OptionalText>(root.second) && *static_cast<CPPUI::OptionalText>(root.first) == "a"
                && selected(stack) == 0 && root.writes == 2, "rebind wrote the old model or failed to clear selection");
            root.second = CPPUI::OptionalText("missing"); settle();
            root.keys.clear(); root.redraw();
            require(selected(stack) == 0 && *static_cast<CPPUI::OptionalText>(root.second) == "missing", "absent pages rewrote the selection");
            root.keys = {"missing"}; root.redraw();
            require(selected(stack) == 1, "bound missing key was not restored after insertion");
            bool rejected = false;
            try { root.second = CPPUI::OptionalText(""); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && selected(stack) == 1 && editors == 1, "invalid bound selection damaged mounted content");
            root.second = CPPUI::OptionalText("missing"); settle();
            root.external = false; root.redraw();
            require(selected(stack) == 0, "returning to an internal model did not clear selection");
            root.views()->select("missing");
            require(selected(stack) == 1, "replacement internal model could not select a page");
            root.external = true; root.redraw();
            root.action = [&]() { root.show = false; root.redraw(); };
            int removed = 0;
            lv_obj_add_event_cb(stack, deleted, LV_EVENT_DELETE, &removed);
            root.views()->select(); settle();
            require(removed == 1 && editors == 0, "selection setter self-removal retained pages");
        }
        {
            std::unique_ptr<Root> root(new Root()); root->external = true; root->redraw();
            root->action = [&root]() { root.reset(); };
            root->views()->select("b"); settle();
            require(!root && editors == 0, "selection setter could not destroy its owning view");
        }
        {
            Root root; root.redraw();
            auto declaration = root.views();
            lv_obj_delete(root.native());
            require(!declaration->ui, "external native deletion left stale Views access");
        }
        settle();
        require(editors == 0 && lv_obj_get_child_count(lv_screen_active()) == 0, "Views teardown leaked native roots or declarations");
        {
            Root root; root.redraw();
            lv_display_delete(display);
            require(!root.views()->ui, "display destruction left a native Views observer");
        }
        require(editors == 0, "display destruction retained view declarations");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

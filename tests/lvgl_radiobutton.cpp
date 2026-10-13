#include "cppui/lvgl/radiobutton.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void count(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
    bool checked(lv_obj_t * object) { return lv_obj_has_state(object, LV_STATE_CHECKED); }
    void click(lv_obj_t * object) {
        if(lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr) == LV_RESULT_OK)
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
    }
    class Root: public CPPUI::LVGL::View {
        public:
        CPPUI::State<int> first{1}, second{2}, separate{1};
        bool other = false, show = true, fail = false;
        int writes = 0, clicks = 0, version = 1;
        std::string text = "A";
        std::function<void()> action;
        std::weak_ptr<int> capture;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) if(show) {
                auto state = other ? &second : &first;
                auto token = std::make_shared<int>(1);
                capture = token;
                CPPUI::Binding<int> binding([this, state]() {
                    if(fail) throw std::runtime_error("expected radio getter failure");
                    return static_cast<int>(*state);
                }, [this, state, token](const int & value) {
                    ++writes;
                    auto invoke = action;
                    *state = value;
                    if(invoke) invoke();
                    require(*token == 1, "active setter lost its captures");
                });
                const auto current = version;
                CPPUI::RadioButton(text, 1, binding).click([this, state, current]() {
                    require(static_cast<int>(*state) == 1, "click ran before radio commit");
                    clicks += current;
                });
                CPPUI::RadioButton("B", 2, binding).click([this, state, current]() {
                    require(static_cast<int>(*state) == 2, "click ran before radio commit");
                    clicks += current;
                });
                for(auto group: CPPUI::VBox()) CPPUI::RadioButton("Separate", 1, separate);
            }
        }
        CPPUI::Node * column() { return children->front()->children->front(); }
        lv_obj_t * button(int index) { return static_cast<lv_obj_t *>(column()->children->at(index)->ui); }
        lv_obj_t * separateButton() { return static_cast<lv_obj_t *>(column()->children->at(2)->children->front()->ui); }
    };
}
int main() {
    lv_init();
    lv_display_create(800, 600);
    int result = 0;
    try {
        int external = 0, deleted = 0;
        {
            Root root;
            root.redraw();
            auto first = root.button(0), second = root.button(1);
            require(checked(first) && !checked(second) && checked(root.separateButton()), "initial value selection failed");
            lv_obj_add_event_cb(second, count, LV_EVENT_CLICKED, &external);
            lv_obj_add_event_cb(first, count, LV_EVENT_DELETE, &deleted);
            auto capture = root.capture;
            lv_obj_send_event(second, LV_EVENT_RELEASED, nullptr);
            require(!checked(first) && checked(second) && checked(root.separateButton()) && root.writes == 0,
                "native sibling exclusion crossed groups or wrote before click");
            lv_obj_send_event(second, LV_EVENT_CLICKED, nullptr);
            require(static_cast<int>(root.first) == 2 && root.writes == 1 && root.clicks == 1 && external == 1
                && root.button(1) == second && capture.expired(), "radio edit lost reuse/listeners or duplicated a commit");
            root.first = 1;
            require(checked(first) && !checked(second) && root.writes == 1, "programmatic selection caused feedback");
            root.other = true; root.version = 2; root.text = "Rebound"; root.redraw();
            require(checked(second) && std::string(lv_checkbox_get_text(first)) == "Rebound", "rebind missed selection/text");
            click(first);
            require(static_cast<int>(root.first) == 1 && static_cast<int>(root.second) == 1
                && root.writes == 2 && root.clicks == 3, "rebind used an old model or callback");
            for(int i = 0; i < 12; ++i) root.redraw();
            click(first);
            require(checked(first) && root.writes == 3 && root.clicks == 5 && root.button(0) == first,
                "repeated selection unchecked radio or accumulated callbacks");
            root.second = 99;
            require(!checked(first) && !checked(second) && root.writes == 3, "unmatched model retained a selection or caused feedback");
            uint32_t key = LV_KEY_UP;
            lv_obj_send_event(second, LV_EVENT_KEY, &key);
            require(static_cast<int>(root.second) == 2 && checked(second) && root.writes == 4 && root.clicks == 5,
                "keyboard selection failed or emitted a click");
            key = LV_KEY_DOWN;
            lv_obj_send_event(second, LV_EVENT_KEY, &key);
            require(checked(second) && root.writes == 4, "keyboard deselected an active radio");
            root.fail = true;
            bool threw = false;
            try { root.redraw(); } catch(const std::runtime_error &) { threw = true; }
            require(threw && root.button(1) == second, "getter failure discarded mounted UI");
            root.fail = false; root.redraw();
            root.action = []() { throw std::runtime_error("expected radio setter failure"); };
            click(first);
            require(root.writes == 5 && static_cast<int>(root.second) == 1, "setter exception escaped native dispatch");
            root.action = [&root]() { root.show = false; root.redraw(); };
            capture = root.capture;
            click(second);
            require(root.column()->children->empty() && capture.expired() && deleted == 1
                && root.writes == 6 && root.clicks == 7, "setter self-removal lost callback or leaked native/capture data");
            root.action = {}; root.show = true; root.redraw();
            auto declaration = root.column()->children->front();
            lv_obj_delete(root.button(0));
            require(!declaration->ui, "external native deletion left stale radio access");
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "radio teardown retained native content");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

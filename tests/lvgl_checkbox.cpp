#include "cppui/lvgl/checkbox.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void count(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
    class Root: public CPPUI::LVGL::View {
        public:
        CPPUI::State<bool> first{false}, second{true};
        bool useSecond = false, show = true, fail = false;
        int writes = 0, clicks = 0;
        std::string text = "First";
        std::function<void()> action;
        std::weak_ptr<int> capture;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) if(show) {
                auto token = std::make_shared<int>(1);
                capture = token;
                auto state = useSecond ? &second : &first;
                CPPUI::Binding<bool> binding([this, state]() {
                    if(fail) throw std::runtime_error("expected getter failure");
                    return static_cast<bool>(*state);
                }, [this, state, token](bool value) {
                    ++writes;
                    auto invoke = action;
                    *state = value;
                    if(invoke) invoke();
                    require(*token == 1, "active setter lost its captures");
                });
                CPPUI::Checkbox(text, binding).click([this, state]() {
                    ++clicks;
                    require(static_cast<bool>(*state), "click ran before checkbox commit");
                });
                CPPUI::Checkbox("Mirror", *state);
            }
        }
        CPPUI::Node * column() { return children->front()->children->front(); }
        lv_obj_t * widget(int index = 0) { return static_cast<lv_obj_t *>(column()->children->at(index)->ui); }
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
            auto object = root.widget();
            require(!lv_obj_has_state(object, LV_STATE_CHECKED), "initial binding value was ignored");
            lv_obj_add_event_cb(object, count, LV_EVENT_VALUE_CHANGED, &external);
            lv_obj_add_event_cb(object, count, LV_EVENT_DELETE, &deleted);
            auto capture = root.capture;
            lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr);
            require(static_cast<bool>(root.first) && root.writes == 1 && external == 1, "native edit committed more than once or lost a listener");
            require(root.widget() == object && lv_obj_has_state(root.widget(1), LV_STATE_CHECKED) && capture.expired(), "redraw lost reuse, mirror state or callback cleanup");
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
            require(root.clicks == 1, "checkbox click was not dispatched");
            root.first = false;
            require(!lv_obj_has_state(object, LV_STATE_CHECKED) && root.writes == 1, "programmatic update caused a feedback write");
            root.useSecond = true;
            root.text = "Second";
            root.redraw();
            require(lv_obj_has_state(object, LV_STATE_CHECKED) && std::string(lv_checkbox_get_text(object)) == "Second", "rebind did not update checked state/text");
            lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr);
            require(!static_cast<bool>(root.second) && !static_cast<bool>(root.first) && root.writes == 2, "rebind wrote the previous model");
            root.fail = true;
            bool threw = false;
            try { root.redraw(); } catch(const std::runtime_error &) { threw = true; }
            require(threw && root.widget() == object, "getter failure discarded mounted UI");
            root.fail = false;
            root.redraw();
            root.action = []() { throw std::runtime_error("expected setter failure"); };
            lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr);
            require(root.writes == 3 && static_cast<bool>(root.second), "setter failure escaped native callback boundary");
            root.action = [&root]() { root.show = false; root.redraw(); };
            capture = root.capture;
            lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr);
            require(root.column()->children->empty() && capture.expired() && deleted == 1, "setter self-removal leaked native/callback data");
            root.action = {};
            root.show = true;
            for(int i = 0; i < 20; ++i) { root.redraw(); root.first = !static_cast<bool>(root.first); }
            auto checkbox = root.column()->children->front();
            lv_obj_delete(root.widget());
            require(!checkbox->ui, "external native deletion left a stale checkbox");
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "checkbox teardown retained native content");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

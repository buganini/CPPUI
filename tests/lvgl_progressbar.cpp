#include "cppui/lvgl/progressbar.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/state.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    uint32_t tick = 1;
    uint32_t now() { return tick; }
    void advance(uint32_t milliseconds) { tick += milliseconds; lv_timer_handler(); }
    class Root: public CPPUI::LVGL::View {
        public:
        CPPUI::State<double> progress{0.125};
        double maximum = 0.25;
        bool show = true;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox())
                if(show) CPPUI::ProgressBar(progress, maximum);
        }
        CPPUI::Node * column() { return children->front()->children->front(); }
        lv_obj_t * bar() { return static_cast<lv_obj_t *>(column()->children->front()->ui); }
    };
}
int main() {
    lv_init();
    lv_tick_set_cb(now);
    auto display = lv_display_create(800, 600);
    lv_timer_pause(lv_display_get_refr_timer(display));
    int result = 0;
    try {
        {
            Root root;
            root.redraw();
            auto bar = root.bar();
            require(lv_bar_get_value(bar) == 12 && lv_bar_get_max_value(bar) == 25, "fractional progress was not truncated to hundredths");
            lv_obj_update_layout(lv_screen_active());
            require(lv_obj_get_width(bar) > 0 && lv_obj_get_height(bar) > 0, "progress bar has no visible size");
            root.maximum = 2.5; root.progress = 1.25;
            require(root.bar() == bar && lv_bar_get_value(bar) == 125 && lv_bar_get_max_value(bar) == 250, "state update lost native reuse or range");
            root.progress = -1;
            require(lv_bar_get_value(bar) == 0, "negative progress was not clamped");
            root.progress = 5;
            require(lv_bar_get_value(bar) == 250, "excess progress was not clamped");
            root.maximum = 1; root.redraw();
            require(lv_bar_get_value(bar) == 100, "shrinking range did not clamp the value");
            for(double invalid: {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 1e20}) {
                bool rejected = false;
                try { root.progress = invalid; } catch(const std::out_of_range &) { rejected = true; }
                require(rejected && root.bar() == bar && lv_bar_get_value(bar) == 100, "invalid progress damaged mounted UI");
            }
            root.progress = 0.5;
            for(double invalid: {-0.001, -1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 1e20}) {
                root.maximum = invalid;
                bool rejected = false;
                try { root.redraw(); } catch(const std::out_of_range &) { rejected = true; }
                require(rejected && root.bar() == bar && lv_bar_get_value(bar) == 50, "invalid maximum damaged mounted UI");
            }
            root.maximum = 0; root.redraw();
            auto animation = lv_anim_get(bar, nullptr);
            require(animation && lv_bar_get_mode(bar) == LV_BAR_MODE_RANGE, "zero maximum did not start a busy indicator");
            advance(200);
            auto start = lv_bar_get_start_value(bar);
            require(start > 0 && lv_bar_get_value(bar) - start == 25, "busy indicator did not move");
            for(int i = 0; i < 20; ++i) root.redraw();
            require(lv_anim_get(bar, nullptr) == animation && lv_bar_get_start_value(bar) == start, "redraw restarted the busy animation");
            advance(200);
            require(lv_bar_get_start_value(bar) > start, "busy animation stopped after redraw");
            root.maximum = 2; root.redraw();
            require(!lv_anim_get(bar, nullptr) && lv_bar_get_mode(bar) == LV_BAR_MODE_NORMAL
                && lv_bar_get_value(bar) == 50 && lv_bar_get_max_value(bar) == 200, "determinate transition retained busy work or lost value");
            root.maximum = 0; root.redraw();
            root.show = false; root.redraw(); advance(200);
            require(root.column()->children->empty() && !lv_anim_get(bar, nullptr), "removal retained busy animation");
            root.show = true; root.redraw();
            bar = root.bar();
            auto declaration = root.column()->children->front();
            lv_obj_delete(bar); advance(200);
            require(!declaration->ui && !lv_anim_get(bar, nullptr), "external deletion left native or animation access");
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "progress teardown retained native content");
        {
            Root root;
            root.maximum = 0; root.redraw();
            lv_display_delete(display);
            advance(200);
            require(!root.column()->children->front()->ui, "display deletion left stale progress access");
        }
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

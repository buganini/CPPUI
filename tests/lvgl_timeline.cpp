#include "cppui/lvgl/timeline.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    uint32_t tick = 1;
    uint32_t now() { return tick; }
    void advance(uint32_t milliseconds) { tick += milliseconds; lv_timer_handler(); }
    int timerCount() {
        int count = 0;
        for(auto timer = lv_timer_get_next(nullptr); timer; timer = lv_timer_get_next(timer)) ++count;
        return count;
    }
    int declarations = 0;
    class Timeline: public CPPUI::LVGL::TimelineView {
        public:
        explicit Timeline(double seconds): TimelineView(seconds) { ++declarations; }
        ~Timeline() override { --declarations; }
    };
    class Root: public CPPUI::LVGL::View {
        public:
        bool show = true, reverse = false, removeOnTick = false, fail = false;
        double seconds = 0.01;
        int builds = 0;
        std::string identity = "timer";
        void content() override {
            ++builds;
            if(removeOnTick) show = false;
            for(auto window: CPPUI::Window()) for(auto row: CPPUI::HBox()) {
                if(reverse) CPPUI::Label("tail").id("tail");
                if(show) for(auto timeline: (new Timeline(seconds))->id(identity)) CPPUI::Label(std::to_string(builds));
                if(!reverse) CPPUI::Label("tail").id("tail");
                if(fail) throw std::runtime_error("expected timeline builder failure");
            }
        }
        lv_obj_t * row() { return static_cast<lv_obj_t *>(children->front()->children->front()->ui); }
    };
}
int main() {
    lv_init();
    lv_tick_set_cb(now);
    auto display = lv_display_create(800, 600);
    lv_timer_pause(lv_display_get_refr_timer(display));
    int result = 0;
    try {
        const int baseline = timerCount();
        {
            Root root;
            root.redraw();
            auto row = root.row();
            auto label = lv_obj_get_child(row, 0);
            require(timerCount() == baseline + 1 && lv_obj_get_child_count(row) == 2, "timeline created extra timers or a native wrapper");
            advance(9);
            require(root.builds == 1, "timeline fired before its interval");
            advance(1);
            require(root.builds == 2 && declarations == 1 && std::string(lv_label_get_text(label)) == "2", "timer did not redraw or retained retired declarations");
            root.reverse = true; root.seconds = 60; root.redraw();
            auto builds = root.builds;
            advance(10);
            require(root.builds == builds + 1 && lv_obj_get_child(row, 1) == label && timerCount() == baseline + 1,
                "keyed move lost native order or changed mounted interval");
            root.identity = "replacement"; root.redraw();
            builds = root.builds; advance(10);
            require(root.builds == builds && timerCount() == baseline + 1, "new identity did not restart with the new interval");
            root.identity = "fast"; root.seconds = 0.0011; root.redraw();
            builds = root.builds; advance(1);
            require(root.builds == builds, "fractional milliseconds were rounded down");
            advance(1);
            require(root.builds == builds + 1, "rounded timer did not fire");
            root.fail = true; builds = root.builds; advance(2);
            require(root.builds == builds + 1 && root.row() == row && declarations == 1, "timer exception escaped or discarded mounted content");
            root.fail = false; advance(2);
            require(root.builds == builds + 2, "timer did not recover after a builder exception");
            for(double invalid: {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 1e20}) {
                root.seconds = invalid;
                bool rejected = false;
                try { root.redraw(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && declarations == 1 && timerCount() == baseline + 1, "invalid interval damaged the mounted timer");
            }
            root.seconds = 0.002; root.removeOnTick = true; advance(2);
            builds = root.builds; advance(20);
            require(root.builds == builds && declarations == 0 && timerCount() == baseline, "timer self-removal left scheduled work");
            root.removeOnTick = false;
            for(int i = 0; i < 20; ++i) {
                root.show = true; root.seconds = 0; root.redraw();
                builds = root.builds; advance(1);
                require(root.builds == builds + 1, "zero interval did not run on the next millisecond");
                root.show = false; root.redraw();
            }
            require(timerCount() == baseline && declarations == 0, "repeated removal leaked timers or declarations");
            root.show = true; root.redraw();
            builds = root.builds;
            lv_obj_delete(root.row()); advance(20);
            require(root.builds == builds && timerCount() == baseline, "native parent deletion did not cancel the timer");
        }
        require(declarations == 0 && timerCount() == baseline && lv_obj_get_child_count(lv_screen_active()) == 0,
            "view teardown retained declarations, timers or native content");
        {
            Root root;
            root.redraw();
        }
        advance(20);
        require(declarations == 0 && timerCount() == baseline, "root teardown left timer work");
        {
            Root root;
            root.redraw();
            const auto builds = root.builds;
            lv_display_delete(display); advance(20);
            require(root.builds == builds, "display deletion allowed a timer redraw");
        }
        require(declarations == 0, "display close retained timeline declarations");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

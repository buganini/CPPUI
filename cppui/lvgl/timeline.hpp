#ifndef CPPUI_LVGL_TIMELINE
#define CPPUI_LVGL_TIMELINE

#include "base.hpp"
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>

namespace CPPUI { namespace LVGL {
    class TimelineView: public ::CPPUI::Node {
        struct Runtime {
            View * root;
            lv_obj_t * parent;
            lv_timer_t * timer;
            static void tick(lv_timer_t * timer) {
                auto root = static_cast<Runtime *>(lv_timer_get_user_data(timer))->root;
                // Redraw can retire this declaration, remove the timer, or fail.
                try { root->redraw(); }
                catch(const std::exception & error) {
                    std::fprintf(stderr, "CPPUI LVGL timeline failed: %s\n", error.what());
                } catch(...) {
                    std::fprintf(stderr, "CPPUI LVGL timeline failed: unknown exception\n");
                }
            }
            void stop() {
                if(timer) lv_timer_delete(timer);
                timer = nullptr;
            }
            static void parentDeleted(lv_event_t * event) {
                auto runtime = static_cast<Runtime *>(lv_event_get_user_data(event));
                runtime->parent = nullptr;
                runtime->stop();
            }
            Runtime(View * root, lv_obj_t * parent, uint32_t milliseconds)
                : root(root), parent(parent), timer(lv_timer_create(tick, milliseconds, this)) {
                lv_obj_add_event_cb(parent, parentDeleted, LV_EVENT_DELETE, this);
            }
            ~Runtime() {
                stop();
                if(parent) lv_obj_remove_event_cb_with_user_data(parent, parentDeleted, this);
            }
        };
        uint32_t milliseconds;
        std::unique_ptr<Runtime> runtime;
        public:
        explicit TimelineView(double seconds) {
            if(!std::isfinite(seconds) || seconds < 0
               || seconds * 1000 > std::numeric_limits<int32_t>::max())
                throw std::invalid_argument("TimelineView interval must be finite, nonnegative and fit 32-bit milliseconds");
            if(!dynamic_cast<View *>(cppui_root))
                throw std::logic_error("TimelineView requires an LVGL Application or View");
            milliseconds = std::max(uint32_t(1), static_cast<uint32_t>(std::ceil(seconds * 1000)));
            is_virtual = true;
        }
        void update(::CPPUI::Node * previous) override {
            auto prev = static_cast<TimelineView *>(previous);
            if(prev) runtime = std::move(prev->runtime);
            if(!runtime) {
                auto parent = static_cast<lv_obj_t *>(dom_parent ? dom_parent->ui : nullptr);
                if(!parent) throw std::logic_error("TimelineView requires a mounted native parent");
                runtime.reset(new Runtime(static_cast<View *>(cppui_root), parent, milliseconds));
            }
            ::CPPUI::Node::update(previous);
        }
        void destroy(bool) override { runtime.reset(); }
    };
}}
namespace CPPUI {
    inline LVGL::TimelineView & TimelineView(double seconds) { return *new LVGL::TimelineView(seconds); }
}
#endif

#ifndef CPPUI_LVGL_PROGRESSBAR
#define CPPUI_LVGL_PROGRESSBAR

#include "base.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>

namespace CPPUI { namespace LVGL {
    class ProgressBar: public Node {
        static int32_t hundredths(double value) {
            const double scaled = std::trunc(value * 100.0);
            if(!std::isfinite(scaled) || scaled < std::numeric_limits<int32_t>::min()
               || scaled > std::numeric_limits<int32_t>::max())
                throw std::out_of_range("ProgressBar value exceeds LVGL's integer range");
            return static_cast<int32_t>(scaled);
        }
        static void animateBusy(void * target, int32_t start) {
            auto object = static_cast<lv_obj_t *>(target);
            // Lower the start first so a backwards sweep is not clamped by it.
            lv_bar_set_start_value(object, 0, LV_ANIM_OFF);
            lv_bar_set_value(object, start + 25, LV_ANIM_OFF);
            lv_bar_set_start_value(object, start, LV_ANIM_OFF);
        }
        public:
        int32_t progress, maximum;
        explicit ProgressBar(double progress, double maximum = 1.0)
            : progress(hundredths(progress)), maximum(hundredths(maximum)) {
            if(maximum < 0) throw std::out_of_range("ProgressBar maximum must be nonnegative");
            is_terminal = true; expandX=1;
        }
        void update(Node * previous) override {
            ui = previous && previous->ui ? previous->ui : lv_bar_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            if(maximum == 0) {
                if(!lv_anim_get(object, animateBusy)) {
                    lv_bar_set_range(object, 0, 100);
                    lv_bar_set_mode(object, LV_BAR_MODE_RANGE);
                    lv_anim_t animation;
                    lv_anim_init(&animation);
                    // LVGL cancels animations targeting an object when it is deleted.
                    lv_anim_set_var(&animation, object);
                    lv_anim_set_exec_cb(&animation, animateBusy);
                    lv_anim_set_values(&animation, 0, 75);
                    lv_anim_set_duration(&animation, 1000);
                    lv_anim_set_reverse_duration(&animation, 1000);
                    lv_anim_set_repeat_count(&animation, LV_ANIM_REPEAT_INFINITE);
                    lv_anim_start(&animation);
                }
            } else {
                lv_anim_delete(object, animateBusy);
                lv_bar_set_mode(object, LV_BAR_MODE_NORMAL);
                lv_bar_set_range(object, 0, maximum);
                lv_bar_set_value(object, progress, LV_ANIM_OFF);
            }
        }
    };
}}
namespace CPPUI {
    inline LVGL::ProgressBar & ProgressBar(double progress, double maximum = 1.0) {
        return *new LVGL::ProgressBar(progress, maximum);
    }
}
#endif

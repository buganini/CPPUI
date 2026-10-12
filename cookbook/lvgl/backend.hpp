#ifndef CPPUI_COOKBOOK_LVGL_BACKEND
#define CPPUI_COOKBOOK_LVGL_BACKEND
#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/button.hpp"
#include "../recipes.hpp"

namespace Cookbook { namespace Backend {
    inline void configureRecipes(std::vector<Recipe> &) {}
    const char * const name = "LVGL";
    inline void settle() {
        lv_tick_inc(20);
        lv_timer_handler();
    }
    inline bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::LVGL::Button *>(&node);
        if(!button || button->text != text) return false;
        lv_obj_send_event(static_cast<lv_obj_t *>(button->outer()), LV_EVENT_CLICKED, nullptr);
        return true;
    }
    inline bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto label = dynamic_cast<CPPUI::LVGL::Label *>(&node);
        return label && text == lv_label_get_text(static_cast<lv_obj_t *>(label->outer()));
    }
}}
#endif

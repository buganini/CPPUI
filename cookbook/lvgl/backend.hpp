#ifndef CPPUI_COOKBOOK_LVGL_BACKEND
#define CPPUI_COOKBOOK_LVGL_BACKEND
#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/button.hpp"
#include "../recipes.hpp"
#include "presentation.hpp"
#include <stdexcept>

namespace Cookbook { namespace Backend {
    inline void configureRecipes(std::vector<Recipe> &) {}
#if CPPUI_LVGL_SDL2
    const char * const name = "LVGL SDL2";
#else
    const char * const name = "LVGL X11";
#endif
    inline void settle() {
        // Let the selected native driver supply time and process its events.
        lv_delay_ms(20);
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
    inline bool selectTab(CPPUI::Node &, int, const std::string &) { return false; }
    inline void answerPrompt(const std::string &, bool) {}
    inline bool selectPage(CPPUI::Node & node) {
        auto link = dynamic_cast<Presentation::PageLinkNode *>(&node);
        if(!link) return false;
        auto label = static_cast<lv_obj_t *>(link->outer());
        lv_obj_scroll_to_view_recursive(label, LV_ANIM_OFF);
        lv_obj_send_event(label, LV_EVENT_CLICKED, nullptr);
        return true;
    }
    inline void prepareLayoutCheck(CPPUI::Node &) {}
    inline bool pageVisible(CPPUI::Node & root, const std::string & key) {
        lv_obj_update_layout(lv_screen_active());
        lv_area_t viewport, label;
        lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "menu-scroller")->outer()), &viewport);
        lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "page:" + key)->outer()), &label);
        return label.y1 >= viewport.y1 && label.y2 <= viewport.y2;
    }
    inline void checkLayout(CPPUI::Node & root) {
        lv_obj_update_layout(lv_screen_active());
#if LV_USE_X11
        lv_obj_t * cursor = nullptr;
        for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if(lv_indev_get_type(input) == LV_INDEV_TYPE_POINTER) cursor = lv_indev_get_cursor(input);
        }
        if(!cursor || !lv_obj_is_visible(cursor) || lv_obj_get_parent(cursor) != lv_layer_sys()
           || lv_obj_get_width(cursor) <= 0 || lv_obj_get_height(cursor) <= 0) {
            throw std::runtime_error("X11 must retain a visible mouse cursor above recipe content");
        }
#endif
        auto sidebar = static_cast<lv_obj_t *>(find(root, "menu-container")->outer());
        auto result = static_cast<lv_obj_t *>(find(root, "result")->outer());
        auto scroll = static_cast<lv_obj_t *>(find(root, "menu-scroller")->outer());
        auto list = static_cast<lv_obj_t *>(find(root, "menu-list")->outer());
        if(lv_obj_get_x(sidebar) + lv_obj_get_width(sidebar) > lv_obj_get_x(result)
           || lv_obj_get_width(result) <= lv_obj_get_width(sidebar) || lv_obj_get_height(result) < 200) {
            throw std::runtime_error("cookbook must retain a sidebar beside an expanding result pane");
        }
        if(lv_obj_get_height(list) <= lv_obj_get_content_height(scroll)
           || lv_obj_get_child_count(list) != recipes().size() + 1
           || lv_obj_get_width(list) < 100
           || lv_obj_get_width(lv_obj_get_child(list, 0)) < 100
           || lv_obj_get_height(lv_obj_get_child(list, 0)) > 40) {
            throw std::runtime_error("cookbook page list must scroll and include every recipe");
        }
        if(find(root, "code")) throw std::runtime_error("source code pane must be omitted");
    }
}}
#endif

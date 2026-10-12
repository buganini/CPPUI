#ifndef CPPUI_COOKBOOK_LVGL_BACKEND
#define CPPUI_COOKBOOK_LVGL_BACKEND
#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/button.hpp"
#include "cppui/lvgl/canvas.hpp"
#include "cppui/lvgl/divider.hpp"
#include "cppui/lvgl/tab.hpp"
#include "cppui/lvgl/checkbox.hpp"
#include "../recipes.hpp"
#include "presentation.hpp"
#include <stdexcept>

namespace Cookbook { namespace Backend {
    inline bool editTextField(CPPUI::Node &, const std::string &, bool) { return false; }
    inline void mount(CPPUI::Application & app) { app.redraw(); }
    inline void configureRecipes(std::vector<Recipe> & pages) {
        for(auto & page: pages) if(std::string(page.key) == "canvas") {
            page.render = canvas;
            page.remaining = "";
        } else if(std::string(page.key) == "tab") {
            page.render = tab;
            page.remaining = "";
        } else if(std::string(page.key) == "checkbox") {
            page.render = checkbox;
            page.remaining = "";
        }
    }
#if CPPUI_LVGL_SDL2
    const char * const name = "LVGL SDL2";
#else
    const char * const name = "LVGL X11";
#endif
    inline void settle() {
        // Finish layout before waiting so size/style changes cannot postpone the frame.
        lv_obj_update_layout(lv_screen_active());
        const auto start = lv_tick_get();
        // X11 ticks in 5 ms increments; elapsed wall time can exceed LVGL time.
        lv_delay_ms(LV_DEF_REFR_PERIOD + 1);
        while(lv_tick_elaps(start) <= LV_DEF_REFR_PERIOD) lv_delay_ms(1);
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
    inline bool selectTab(CPPUI::Node & node, int index, const std::string & text) {
        auto tabs = dynamic_cast<CPPUI::LVGL::Tabs *>(&node);
        if(!tabs || !tabs->ui) return false;
        auto object = static_cast<lv_obj_t *>(tabs->ui);
        auto button = lv_tabview_get_tab_button(object, index);
        if(!button) return false;
        lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
        auto page = lv_obj_get_child(lv_tabview_get_content(object), index);
        auto label = lv_obj_get_child(button, 0);
        return lv_tabview_get_tab_active(object) == static_cast<uint32_t>(index)
            && label && text == lv_label_get_text(label) && lv_obj_is_visible(page);
    }
    inline bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::LVGL::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        lv_obj_send_event(static_cast<lv_obj_t *>(checkbox->ui), LV_EVENT_RELEASED, nullptr);
        return true;
    }
    inline void answerPrompt(const std::string &, bool) {}
    inline bool modalVisible(CPPUI::Node &) { return false; }
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
        if(auto row = find(root, "spacer-row")) {
            auto box = static_cast<lv_obj_t *>(row->outer());
            auto gap = static_cast<lv_obj_t *>(find(root, "row-gap")->outer());
            lv_area_t content, end;
            lv_obj_get_content_coords(box, &content);
            lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "row-end")->outer()), &end);
            if(lv_obj_get_width(gap) <= lv_obj_get_content_width(box)/2 || end.x2 != content.x2)
                throw std::runtime_error("horizontal spacer must push Column 2 to the row's far edge");
        }
        if(auto column = find(root, "spacer-column")) {
            auto box = static_cast<lv_obj_t *>(column->outer());
            auto gap = static_cast<lv_obj_t *>(find(root, "column-gap")->outer());
            lv_area_t content, end;
            lv_obj_get_content_coords(box, &content);
            lv_obj_get_coords(gap, &end);
            if(lv_obj_get_height(gap) <= lv_obj_get_content_height(box)/2 || end.y2 != content.y2)
                throw std::runtime_error("vertical spacer must fill the remaining column height");
        }
        if(auto canvas = find(root, "example-canvas")) {
            auto object = static_cast<lv_obj_t *>(canvas->outer());
            auto buffer = lv_canvas_get_draw_buf(object);
            if(lv_obj_get_width(object) != 100 || lv_obj_get_height(object) != 100
               || !buffer || buffer->header.w != 100 || buffer->header.h != 100) {
                throw std::runtime_error("canvas recipe must paint at its fixed 100x100 size");
            }
            const auto pixel = lv_canvas_get_px(object, 15, 15);
            if(pixel.alpha != 255 || pixel.red != 0xcc || pixel.green != 0xcc || pixel.blue != 0xcc) {
                throw std::runtime_error("canvas recipe did not paint its rectangle");
            }
        }
    }
}}
#endif

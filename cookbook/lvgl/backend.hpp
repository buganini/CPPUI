#ifndef CPPUI_COOKBOOK_LVGL_BACKEND
#define CPPUI_COOKBOOK_LVGL_BACKEND
#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/scroll.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/button.hpp"
#include "cppui/lvgl/canvas.hpp"
#include "cppui/lvgl/divider.hpp"
#include "cppui/lvgl/tab.hpp"
#include "cppui/lvgl/views.hpp"
#include "cppui/lvgl/textfield.hpp"
#include "cppui/lvgl/text.hpp"
#include "cppui/lvgl/checkbox.hpp"
#include "cppui/lvgl/radiobutton.hpp"
#include "cppui/lvgl/progressbar.hpp"
#include "cppui/lvgl/timeline.hpp"
#include "../recipes.hpp"
#include <stdexcept>

namespace Cookbook { namespace Backend {
    using View = CPPUI::LVGL::View;
    using Views = CPPUI::LVGL::Views;
    inline bool viewSelected(CPPUI::Node & node, const std::string & key) {
        auto views = dynamic_cast<Views *>(&node);
        if(!views || !views->ui) return false;
        bool found = key.empty();
        for(auto child: *views->children) {
            const bool visible = !lv_obj_is_hidden(static_cast<lv_obj_t *>(child->outer()));
            if(visible != (!key.empty() && child->_id == key)) return false;
            if(visible) found = true;
        }
        return found;
    }
    inline void configureWindow(CPPUI::Context *) {}
    inline void menus() {}
    inline int treeChildCount(CPPUI::Node &, const std::vector<int> &) { return -1; }
    inline bool treeExpanded(CPPUI::Node &, const std::vector<int> &) { return false; }
    inline std::string tableText(CPPUI::Node &, int, int) { return {}; }
    inline bool editTable(CPPUI::Node &, int, int, const std::string &) { return false; }
    inline bool atScrollEnd(CPPUI::Node &) { return false; }
    inline bool selectCombo(CPPUI::Node &, int) { return false; }
    inline bool editTextField(CPPUI::Node & node, const std::string & text, bool commit) {
        auto field = dynamic_cast<CPPUI::LVGL::TextField *>(&node);
        if(!field || !field->ui) return false;
        auto object = static_cast<lv_obj_t *>(field->ui);
        auto group = static_cast<lv_group_t *>(lv_obj_get_group(object));
        if(!group) return false;
        lv_group_focus_obj(object);
        lv_textarea_set_text(object, text.c_str());
        if(commit) lv_group_send_data(group, LV_KEY_ENTER);
        return true;
    }
    inline void mount(CPPUI::Application & app) { app.redraw(); }
    inline void configureRecipes(std::vector<Recipe> & pages) {
        for(auto & page: pages) if(std::string(page.key) == "canvas") {
            page.render = canvas;
            page.remaining = "";
        } else if(std::string(page.key) == "tab") {
            page.render = tab;
            page.remaining = "";
        } else if(std::string(page.key) == "views") {
            page.render = views;
            page.remaining = "";
        } else if(std::string(page.key) == "textfield") {
            page.render = textfield;
            page.remaining = "String models; dynamic numeric conversion and typed input/change payloads pending";
        } else if(std::string(page.key) == "text") {
            page.render = [](Model & model) {
                CPPUI::Text("HTML and Markdown formatting is not yet implemented; raw source is shown below.");
                text(model);
            };
            page.remaining = "Plain multiline text implemented; HTML and Markdown display raw source; selection not yet implemented";
        } else if(std::string(page.key) == "checkbox") {
            page.render = checkbox;
            page.remaining = "";
        } else if(std::string(page.key) == "radiobutton") {
            page.render = radiobutton;
            page.remaining = "";
        } else if(std::string(page.key) == "progressbar") {
            page.render = progressbar;
            page.remaining = "";
        } else if(std::string(page.key) == "timeline") {
            page.render = timeline;
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
        // Wait for the next refresh using the backend's elapsed-time clock.
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
        auto multiline = dynamic_cast<CPPUI::LVGL::Text *>(&node);
        return (label || multiline) && node.outer()
            && text == lv_label_get_text(static_cast<lv_obj_t *>(node.outer()));
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
    inline bool selectRadio(CPPUI::Node & node) {
        auto radio = dynamic_cast<CPPUI::LVGL::Node *>(&node);
        auto object = radio ? static_cast<lv_obj_t *>(radio->ui) : nullptr;
        if(!object || !lv_obj_is_radio_button(object)) return false;
        if(lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr) == LV_RESULT_OK)
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
        return true;
    }
    inline void answerPrompt(const std::string &, bool) {}
    inline bool modalVisible(CPPUI::Node &) { return false; }
    inline bool selectPage(CPPUI::Node & node) {
        auto link = dynamic_cast<CPPUI::LVGL::Label *>(&node);
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
        bool keyboardReady = false;
        for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if(lv_indev_get_type(input) == LV_INDEV_TYPE_KEYPAD && lv_group_get_default()
               && lv_indev_get_group(input) == lv_group_get_default()) keyboardReady = true;
        }
        if(!keyboardReady) throw std::runtime_error("cookbook keyboard must be attached to the default focus group");
#if LV_USE_X11
        lv_indev_t * pointer = nullptr;
        for(auto input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
            if(lv_indev_get_type(input) == LV_INDEV_TYPE_POINTER) pointer = input;
        }
        if(!pointer || lv_indev_get_cursor(pointer)) {
            throw std::runtime_error("X11 must retain pointer input without an LVGL software cursor");
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

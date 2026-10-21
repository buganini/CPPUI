#include "../cookbook_backend.hpp"
#include <stdexcept>
#if LV_USE_SDL
#include <SDL.h>
#elif LV_USE_X11
#include <X11/Xlib.h>
#endif

namespace Cookbook { namespace TestBackend {
    void dragDivider(lv_obj_t * handle,int dx,int dy) {
        lv_area_t area;lv_obj_get_coords(handle,&area);
        int x=(area.x1+area.x2)/2,y=(area.y1+area.y2)/2;
        auto display=lv_obj_get_display(handle);
        for(int step=0;step<4;++step) {
            if(step==2) {x+=dx;y+=dy;}
#if LV_USE_SDL
            const auto window=SDL_GetWindowID(lv_sdl_window_get_window(display));
            SDL_Event event{};
            if(step==0) {
                event.type=SDL_WINDOWEVENT;event.window.windowID=window;event.window.event=SDL_WINDOWEVENT_ENTER;
                SDL_PushEvent(&event);event=SDL_Event{};
            }
            if(step==0 || step==2) {
                event.type=SDL_MOUSEMOTION;event.motion.windowID=window;
                event.motion.x=x;event.motion.y=y;event.motion.state=step==2 ? SDL_BUTTON_LMASK : 0;
            } else {
                event.type=step==1 ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
                event.button.windowID=window;event.button.button=SDL_BUTTON_LEFT;
                event.button.state=step==1 ? SDL_PRESSED : SDL_RELEASED;event.button.x=x;event.button.y=y;
            }
            SDL_PushEvent(&event);
#elif LV_USE_X11
            auto connection=static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display))->display;
            XEvent event{};
            if(step==0 || step==2) {event.type=MotionNotify;event.xmotion.x=x;event.xmotion.y=y;}
            else {event.type=step==1 ? ButtonPress : ButtonRelease;event.xbutton.button=Button1;event.xbutton.x=x;event.xbutton.y=y;}
            XPutBackEvent(connection,&event);
            if(step==0) {event=XEvent{};event.type=EnterNotify;XPutBackEvent(connection,&event);}
#endif
            settle();settle();
        }
    }
    bool viewSelected(CPPUI::Node & node, const std::string & key) {
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
    int treeChildCount(CPPUI::Node &, const std::vector<int> &) { return -1; }
    bool treeExpanded(CPPUI::Node &, const std::vector<int> &) { return false; }
    std::string tableText(CPPUI::Node & node,int row,int column) {
        auto table=dynamic_cast<CPPUI::LVGL::Table *>(&node);
        return table ? table->cellText(row,column) : std::string();
    }
    bool editTable(CPPUI::Node & node,int row,int column,const std::string & value) {
        auto table=dynamic_cast<CPPUI::LVGL::Table *>(&node);
        return table && table->edit(row,column,value);
    }
    bool atScrollEnd(CPPUI::Node &) { return false; }
    bool selectCombo(CPPUI::Node & node, int index) {
        auto combo = dynamic_cast<CPPUI::LVGL::ComboBox *>(&node);
        if(!combo || !combo->ui || index < 0) return false;
        auto object = static_cast<lv_obj_t *>(combo->ui);
        auto dropdown = lv_obj_check_type(object, &lv_dropdown_class) ? object : lv_obj_get_child(object, 1);
        if(dropdown) lv_dropdown_open(dropdown);
        if(!dropdown || static_cast<unsigned>(index) >= lv_dropdown_get_option_count(dropdown)) return false;
        lv_dropdown_set_selected(dropdown, index);
        lv_dropdown_close(dropdown);
        lv_obj_send_event(dropdown, LV_EVENT_VALUE_CHANGED, nullptr);
        return true;
    }
    bool typeCombo(CPPUI::Node & node, const std::string & text) {
        auto combo = dynamic_cast<CPPUI::LVGL::ComboBox *>(&node);
        if(!combo || !combo->ui) return false;
        auto editor = lv_obj_get_child(static_cast<lv_obj_t *>(combo->ui), 0);
        if(!editor || !lv_obj_check_type(editor, &lv_textarea_class)) return false;
        lv_textarea_set_text(editor, text.c_str()); return true;
    }
    bool wheelCanvas(CPPUI::Node & node, int delta) {
        auto canvas = dynamic_cast<CPPUI::LVGL::Canvas *>(&node);
        if(!canvas || !canvas->ui) return false;
        auto object = static_cast<lv_obj_t *>(canvas->ui);
        lv_obj_update_layout(object);
        lv_area_t area; lv_obj_get_coords(object, &area);
        const int x = (area.x1 + area.x2)/2, y = area.y1 + 40;
        auto display = lv_obj_get_display(object);
        // Feed the actual driver queue so smoke tests cover SDL's pointer wheel
        // and X11's separate encoder, including the native input bridge.
#if LV_USE_SDL
        const auto window = SDL_GetWindowID(lv_sdl_window_get_window(display));
        SDL_Event event{}; event.type = SDL_WINDOWEVENT;
        event.window.windowID = window; event.window.event = SDL_WINDOWEVENT_ENTER;
        SDL_PushEvent(&event);
        event = SDL_Event{}; event.type = SDL_MOUSEMOTION;
        event.motion.windowID = window; event.motion.x = x; event.motion.y = y;
        SDL_PushEvent(&event); settle();
        event = SDL_Event{}; event.type = SDL_MOUSEWHEEL;
        event.wheel.windowID = window; event.wheel.y = delta/120;
        SDL_PushEvent(&event);
#elif LV_USE_X11
        auto connection = static_cast<_x11_user_hdr_t *>(lv_display_get_driver_data(display))->display;
        XEvent event{}; event.type = MotionNotify; event.xmotion.x = x; event.xmotion.y = y;
        XPutBackEvent(connection, &event);
        event = XEvent{}; event.type = EnterNotify; XPutBackEvent(connection, &event);
        settle(); settle(); // Publish motion before the independent wheel device reads.
        event = XEvent{}; event.type = ButtonPress; event.xbutton.button = delta > 0 ? Button4 : Button5;
        XPutBackEvent(connection, &event);
#endif
        settle(); settle();
        return true;
    }
    bool editTextField(CPPUI::Node & node, const std::string & text, bool commit) {
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
    void settle() {
        // Finish layout before waiting so size/style changes cannot postpone the frame.
        lv_obj_update_layout(lv_screen_active());
        const auto start = lv_tick_get();
        // Wait for the next refresh using the backend's elapsed-time clock.
        lv_delay_ms(LV_DEF_REFR_PERIOD + 1);
        while(lv_tick_elaps(start) <= LV_DEF_REFR_PERIOD) lv_delay_ms(1);
        lv_timer_handler();
    }
    bool clickButton(CPPUI::Node & node, const std::string & text) {
        auto button = dynamic_cast<CPPUI::LVGL::Button *>(&node);
        if(!button || button->text != text) return false;
        lv_obj_send_event(static_cast<lv_obj_t *>(button->outer()), LV_EVENT_CLICKED, nullptr);
        return true;
    }
    bool labelHasText(CPPUI::Node & node, const std::string & text) {
        auto label = dynamic_cast<CPPUI::LVGL::Label *>(&node);
        auto multiline = dynamic_cast<CPPUI::LVGL::Text *>(&node);
        return (label || multiline) && node.outer()
            && text == lv_label_get_text(static_cast<lv_obj_t *>(node.outer()));
    }
    bool selectTab(CPPUI::Node & node, int index, const std::string & text) {
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
    bool toggleCheckbox(CPPUI::Node & node) {
        auto checkbox = dynamic_cast<CPPUI::LVGL::Checkbox *>(&node);
        if(!checkbox || !checkbox->ui) return false;
        lv_obj_send_event(static_cast<lv_obj_t *>(checkbox->ui), LV_EVENT_RELEASED, nullptr);
        return true;
    }
    bool selectRadio(CPPUI::Node & node) {
        auto radio = dynamic_cast<CPPUI::LVGL::Node *>(&node);
        auto object = radio ? static_cast<lv_obj_t *>(radio->ui) : nullptr;
        if(!object || !lv_obj_is_radio_button(object)) return false;
        if(lv_obj_send_event(object, LV_EVENT_RELEASED, nullptr) == LV_RESULT_OK)
            lv_obj_send_event(object, LV_EVENT_CLICKED, nullptr);
        return true;
    }
    void answerPrompt(const std::string &, bool) {}
    bool modalVisible(CPPUI::Node &) { return false; }
    bool selectPage(CPPUI::Node & node) {
        auto link = dynamic_cast<CPPUI::LVGL::Label *>(&node);
        if(!link) return false;
        auto label = static_cast<lv_obj_t *>(link->outer());
        lv_obj_scroll_to_view_recursive(label, LV_ANIM_OFF);
        lv_obj_send_event(label, LV_EVENT_CLICKED, nullptr);
        return true;
    }
    void prepareLayoutCheck(CPPUI::Node & root) {
        auto display=lv_display_get_default();
        if(lv_display_get_horizontal_resolution(display)!=1024 || lv_display_get_vertical_resolution(display)!=768)
            throw std::runtime_error("cookbook should open at 1024x768");
        // Exercise the first visit before resizing or revisiting the page.
        // Let native focus/scroll animations finish using the normal event loop.
        selectPage(*find(root,"page:adaptive_grid"));
        const auto start=lv_tick_get();
        do { lv_timer_handler();lv_delay_ms(5); } while(lv_tick_elaps(start)<500);
        auto grid=static_cast<lv_obj_t *>(find(root,"adaptive-grid")->outer());
        auto viewport=lv_obj_get_parent(grid);
        // The Code pane makes the initial result narrow enough for the grid
        // to overflow vertically. Its first tile must still start in view.
        if(lv_obj_get_height(grid)<=0 || lv_obj_get_content_height(viewport)<=0
           || lv_obj_get_scroll_y(viewport)!=0
           || !lv_obj_is_visible(static_cast<lv_obj_t *>(find(root,"A")->outer())))
            throw std::runtime_error("adaptive grid must be visible on its first visit without a resize");
        selectPage(*find(root,"page:adaptive"));settle();
        lv_display_set_resolution(display,800,600);settle();
    }
    void resizeWindow(CPPUI::Node &,int width,int height) {
        lv_display_set_resolution(lv_display_get_default(),width,height);
    }
    void resizeCodePane(CPPUI::Node & root,int width) {
        auto panes=static_cast<lv_obj_t *>(find(root,"example-panes")->outer());
        dragDivider(lv_obj_get_child(panes,1),width-lv_obj_get_width(lv_obj_get_child(panes,0)),0);
    }
    bool pageVisible(CPPUI::Node & root, const std::string & key) {
        lv_obj_update_layout(lv_screen_active());
        lv_area_t viewport, label;
        lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "menu-scroller")->outer()), &viewport);
        lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "page:" + key)->outer()), &label);
        return label.y1 >= viewport.y1 && label.y2 <= viewport.y2;
    }
    void checkLayout(CPPUI::Node & root) {
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
        auto code = static_cast<lv_obj_t *>(find(root, "code")->outer());
        auto panes = static_cast<lv_obj_t *>(find(root, "example-panes")->outer());
        auto scroll = static_cast<lv_obj_t *>(find(root, "menu-scroller")->outer());
        auto list = static_cast<lv_obj_t *>(find(root, "menu-list")->outer());
        lv_area_t sidebarArea,codeArea,resultArea;
        lv_obj_get_coords(sidebar,&sidebarArea);lv_obj_get_coords(code,&codeArea);lv_obj_get_coords(result,&resultArea);
        if(sidebarArea.x2 >= codeArea.x1 || codeArea.x2 >= resultArea.x1
           || lv_obj_get_width(code) < 100 || lv_obj_get_width(result) < 150 || lv_obj_get_height(result) < 200
           || lv_obj_get_child_count(panes) != 3) {
            throw std::runtime_error("cookbook must keep the menu beside the Code/Result splitter");
        }
        if(lv_obj_get_height(list) <= lv_obj_get_content_height(scroll)
           || lv_obj_get_child_count(list) != recipes().size() + 1
           || lv_obj_get_width(list) < 100
           || lv_obj_get_width(lv_obj_get_child(list, 0)) < 100
           || lv_obj_get_height(lv_obj_get_child(list, 0)) > 40) {
            throw std::runtime_error("cookbook page list must scroll and include every recipe");
        }
        if(auto split=find(root,"example-splitter")) {
            auto outer=static_cast<lv_obj_t *>(split->outer());
            auto inner=static_cast<lv_obj_t *>(find(root,"splitter-horizontal")->outer());
            if(lv_obj_get_child_count(outer)!=3 || lv_obj_get_child_count(inner)!=5
               || lv_obj_get_height(inner)<100 || lv_obj_get_width(inner)<200)
                throw std::runtime_error("Splitter recipe must mount three horizontal panes inside a vertical split");
            auto left=lv_obj_get_child(inner,0),right=lv_obj_get_child(inner,2);
            const int leftWidth=lv_obj_get_width(left),rightWidth=lv_obj_get_width(right),topHeight=lv_obj_get_height(inner);
            dragDivider(lv_obj_get_child(inner,1),25,0);
            if(lv_obj_get_width(left)!=leftWidth+25 || lv_obj_get_width(right)!=rightWidth-25)
                throw std::runtime_error("native driver did not drag the horizontal splitter handle");
            dragDivider(lv_obj_get_child(outer,1),0,20);
            if(lv_obj_get_height(inner)!=topHeight+20)
                throw std::runtime_error("native driver did not drag the vertical splitter handle");
        }
        if(auto row = find(root, "spacer-row")) {
            auto box = static_cast<lv_obj_t *>(row->outer());
            auto gap = static_cast<lv_obj_t *>(find(root, "row-gap")->outer());
            lv_area_t content, end;
            lv_obj_get_content_coords(box, &content);
            lv_obj_get_coords(static_cast<lv_obj_t *>(find(root, "row-end")->outer()), &end);
            if(lv_obj_get_width(gap) <= 0 || end.x2 != content.x2)
                throw std::runtime_error("horizontal spacer must push Column 2 to the row's far edge");
        }
        if(auto column = find(root, "spacer-column")) {
            auto box = static_cast<lv_obj_t *>(column->outer());
            auto gap = static_cast<lv_obj_t *>(find(root, "column-gap")->outer());
            lv_area_t content, end;
            lv_obj_get_content_coords(box, &content);
            lv_obj_get_coords(gap, &end);
            if(lv_obj_get_height(gap) <= 0 || end.y2 != content.y2)
                throw std::runtime_error("vertical spacer must fill the remaining column height");
        }
        if(auto grid = find(root, "example-grid")) {
            auto object = static_cast<lv_obj_t *>(grid->outer());
            auto native=[&](const char * id) {
                auto node=find(*grid,id);
                if(!node || !node->outer()) throw std::runtime_error("Manual Grid tile is missing");
                return static_cast<lv_obj_t *>(node->outer());
            };
            auto a=native("A"),b=native("B"),c=native("C"),d=native("D"),e=native("E");
            if(lv_obj_get_x(a)!=0 || lv_obj_get_y(a)!=0 || lv_obj_get_y(b)!=0
               || lv_obj_get_x(b)!=lv_obj_get_width(a)+6 || lv_obj_get_x(c)!=lv_obj_get_x(b)
               || lv_obj_get_y(c)!=lv_obj_get_height(b)+6 || lv_obj_get_y(d)!=lv_obj_get_y(c)
               || lv_obj_get_x(d)!=lv_obj_get_x(c)+lv_obj_get_width(c)+6
               || lv_obj_get_x(e)!=0 || lv_obj_get_y(e)!=lv_obj_get_height(a)+6
               || lv_obj_get_width(b)!=lv_obj_get_width(c)+6+lv_obj_get_width(d)
               || lv_obj_get_width(e)!=lv_obj_get_width(a)+6+lv_obj_get_width(c)
               || lv_obj_get_y(d)+lv_obj_get_height(d)!=lv_obj_get_y(e)+lv_obj_get_height(e)
               || lv_obj_get_height(object)!=lv_obj_get_y(e)+lv_obj_get_height(e))
                throw std::runtime_error("Manual Grid mosaic must preserve its three tracks and spanning tiles");
        }
        if(auto stack = find(root, "example-stack")) {
            auto object = static_cast<lv_obj_t *>(stack->outer());
            auto bottom = static_cast<lv_obj_t *>(find(root, "stack-bottom")->outer());
            auto top = static_cast<lv_obj_t *>(find(root, "stack-top")->outer());
            lv_area_t a, b, content;
            lv_obj_get_coords(bottom, &a); lv_obj_get_coords(top, &b);
            lv_obj_get_content_coords(object, &content);
            if(a.x1 != b.x1 || a.y1 != b.y1 || a.x1 != content.x1 || a.y1 != content.y1
               || lv_obj_get_height(object) != lv_obj_get_height(bottom)
               || lv_obj_get_height(bottom) < lv_obj_get_height(top)
               || lv_obj_get_height(top) != lv_obj_get_style_text_font(top, LV_PART_MAIN)->line_height
               || lv_obj_get_child(object, 1) != top)
                throw std::runtime_error("Stack recipe must overlap natural-height labels with the last layer on top");
        }
        if(auto image = find(root, "example-image")) {
            auto object = static_cast<lv_obj_t *>(image->outer());
            auto pixels = static_cast<const lv_draw_buf_t *>(lv_image_get_src(object));
            if(!pixels || pixels->header.w != 400 || pixels->header.h != 339
               || lv_obj_get_width(object) != 400 || lv_obj_get_height(object) != 339)
                throw std::runtime_error("Image recipe must decode the windmill and preserve its aspect ratio");
        }
        if(auto canvas = find(root, "example-canvas")) {
            auto object = static_cast<lv_obj_t *>(canvas->outer());
            auto buffer = lv_canvas_get_draw_buf(object);
            if(lv_obj_get_width(object) != 100 || lv_obj_get_height(object) != 100
               || !buffer || buffer->header.w != 100 || buffer->header.h != 100) {
                throw std::runtime_error("canvas recipe must paint at its fixed 100x100 size");
            }
            const auto pixel = lv_canvas_get_px(object, 15, 15);
            if(pixel.alpha != 255 || pixel.red != 0xf2 || pixel.green != 0xe6 || pixel.blue != 0xcf) {
                throw std::runtime_error("canvas recipe did not paint its rectangle");
            }
        }
        if(auto canvas = find(root, "heavy-canvas")) {
            auto object = static_cast<lv_obj_t *>(canvas->outer());
            auto buffer = lv_canvas_get_draw_buf(object);
            if(!buffer || buffer->header.w < 320 || buffer->header.h != 720)
                throw std::runtime_error("HeavyCanvas must paint a usable chart at its requested height");
            if(lv_canvas_get_px(object, 70, 24).alpha == 0)
                throw std::runtime_error("HeavyCanvas did not render its price grid");
        }
    }
}}

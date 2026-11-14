#ifndef CPPUI_LVGL_BUTTON
#define CPPUI_LVGL_BUTTON

#include "base.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace CPPUI {
    namespace LVGL {
        class Button: public Node {
            bool isDisabled = false;
            // Native children are owned by the button object; observe their
            // deletion so retained declarations never hold dangling pointers.
            static void partDeleted(lv_event_t * event) {
                auto self = static_cast<Button *>(lv_event_get_user_data(event));
                auto target = lv_event_get_target_obj(event);
                if(target == self->label) self->label = nullptr;
                if(target == self->image) { self->image = nullptr; self->appliedIcon.clear(); }
            }
            void unobserveParts() {
                if(label) lv_obj_remove_event_cb_with_user_data(label, partDeleted, this);
                if(image) lv_obj_remove_event_cb_with_user_data(image, partDeleted, this);
            }
            void observeParts() {
                if(label) lv_obj_add_event_cb(label, partDeleted, LV_EVENT_DELETE, this);
                if(image) lv_obj_add_event_cb(image, partDeleted, LV_EVENT_DELETE, this);
            }
            static std::string iconSource(const std::string & path) {
#if LV_USE_FS_STDIO && LV_FS_STDIO_LETTER
                return std::string(1, LV_FS_STDIO_LETTER) + ":" + path;
#else
                throw std::runtime_error("Button icon requires LV_USE_FS_STDIO and a nonzero LV_FS_STDIO_LETTER: " + path);
#endif
            }
            void applyIcon(lv_obj_t * object) {
                if(iconPath.empty()) {
                    if(image) lv_obj_delete(image); // partDeleted clears the pointer.
                    return;
                }
                if(!image) {
                    image = lv_image_create(object);
                    lv_obj_add_event_cb(image, partDeleted, LV_EVENT_DELETE, this);
                    lv_obj_move_to_index(image, 0);
                    lv_obj_set_clickable(image, false);
                }
                if(appliedIcon == iconPath) return;
                const auto source = iconSource(iconPath);
                lv_image_set_src(image, source.c_str());
                appliedIcon = iconPath;
                // Fit the icon into a 20-pixel square, preserving aspect ratio.
                const int32_t width = lv_image_get_src_width(image), height = lv_image_get_src_height(image);
                if(width > 0 && height > 0) {
                    const double scale = 20.0 / std::max(width, height);
                    lv_obj_set_size(image, std::max<int32_t>(1, std::lround(width * scale)), std::max<int32_t>(1, std::lround(height * scale)));
                    lv_image_set_inner_align(image, LV_IMAGE_ALIGN_CONTAIN);
                    lv_image_set_antialias(image, true);
                }
            }
            public:
            lv_obj_t * label = nullptr;
            lv_obj_t * image = nullptr;
            std::string appliedIcon;
            std::string text;
            std::string iconPath;
            explicit Button(const std::string & text = ""): text(text) {
                is_terminal = true;
                clickable = true;
            }
            ~Button() override { unobserveParts(); }
            Button & icon(const std::string & path) { iconPath = path; return *this; }
            Button & disabled(bool value = true) { isDisabled = value; return *this; }
            void destroy(bool direct) override {
                unobserveParts();
                label = image = nullptr;
                Node::destroy(direct);
            }
            void update(Node * prev) override {
                auto previous = static_cast<Button *>(prev);
                if(previous && previous->ui) {
                    ui = previous->ui;
                    previous->unobserveParts();
                    label = previous->label; image = previous->image; appliedIcon = previous->appliedIcon;
                    previous->label = previous->image = nullptr;
                    observeParts();
                } else {
                    ui = lv_button_create(static_cast<lv_obj_t *>(parent->inner()));
                    auto object = static_cast<lv_obj_t *>(ui);
                    lv_obj_set_flex_flow(object, LV_FLEX_FLOW_ROW);
                    lv_obj_set_flex_align(object, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                    lv_obj_set_style_pad_column(object, 6, 0);
                }
                auto object = static_cast<lv_obj_t *>(ui);
                lv_obj_set_state(object, LV_STATE_DISABLED, isDisabled);
                applyIcon(object);
                if(!text.empty() || iconPath.empty()) {
                    if(!label) {
                        label = lv_label_create(object);
                        lv_obj_add_event_cb(label, partDeleted, LV_EVENT_DELETE, this);
                    }
                    lv_label_set_text(label, text.c_str());
                } else if(label) lv_obj_delete(label); // partDeleted clears the pointer.
            }
        };
    }
    inline LVGL::Button & Button(const std::string & text = "") { return *new LVGL::Button(text); }
}

#endif

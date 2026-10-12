#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/scroll.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/button.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    CPPUI::Node * find(CPPUI::Node & node, const char * id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) if(auto match = find(*child, id)) return match;
        return nullptr;
    }
    class Root: public CPPUI::Application {
        public:
        using Application::Application;
        int phase = 0;
        void content() override {
            for(auto window: CPPUI::Window()) {
                auto & column = CPPUI::VBox(); column.id("box");
                if(phase == 0) column.layout({}, {}, {}, CPPUI::Insets(1, 2, 3, 4));
                if(phase == 1) column.layout({}, {}, {}, CPPUI::Insets(0));
                for(auto scope: column) {
                    auto & label = CPPUI::Label("text"); label.id("label");
                    if(phase == 0) label.layout(160, 45, 2).style(0xff0000, 0x123456, 20);
                    if(phase == 1) label.layout(0, 0, 0).style(0, 0);
                    CPPUI::Button("styled").style(0xffff00, {}, 20).id("button");
                    const auto vertical = phase == 0 ? CPPUI::Optional<bool>() : CPPUI::Optional<bool>(phase == 1);
                    for(auto scroll: CPPUI::Scroll(vertical).layout(200, 100).id("scroll"))
                        for(auto content: CPPUI::VBox()) for(int i = 0; i < 30; ++i) CPPUI::Label("row");
                }
            }
        }
        lv_obj_t * native(const char * id) { return static_cast<lv_obj_t *>(find(*this, id)->outer()); }
    };
}
int main(int argc, char ** argv) {
    lv_init(); lv_display_create(640, 480);
    int result = 0;
    try {
        bool rejected = false;
        try { CPPUI::LVGL::Scroll unsupported(true, {}); }
        catch(const std::invalid_argument &) { rejected = true; }
        require(rejected, "unsupported mixed scrollbar modes were silently accepted");
        {
            Root root(argc, argv); root.redraw(); lv_obj_update_layout(lv_screen_active());
            auto label = root.native("label");
            require(lv_obj_get_width(label) == 160 && lv_obj_get_style_flex_grow(label, LV_PART_MAIN) == 2, "dimensions/weight were ignored");
            require(lv_obj_get_style_pad_left(root.native("box"), LV_PART_MAIN) == 4 && lv_obj_get_style_pad_bottom(root.native("box"), LV_PART_MAIN) == 3, "padding order differs from PUI");
            require(lv_color_to_u32(lv_obj_get_style_bg_color(label, LV_PART_MAIN)) == 0xff123456, "background style was not applied");
            require(lv_obj_get_style_text_font(label, LV_PART_MAIN) == &lv_font_montserrat_28, "configured font size was not selected");
            auto buttonLabel = static_cast<CPPUI::LVGL::Button *>(find(root, "button"))->label;
            require(lv_color_to_u32(lv_obj_get_style_text_color(buttonLabel, LV_PART_MAIN)) == 0xffffff00, "button text did not inherit its style");
            require(lv_obj_get_scroll_bottom(root.native("scroll")) > 0, "public Scroll did not overflow");
            require(lv_obj_get_scrollbar_mode(root.native("scroll")) == LV_SCROLLBAR_MODE_AUTO, "default scrollbar mode was not automatic");
            root.phase = 1; root.redraw(); lv_obj_update_layout(lv_screen_active());
            require(root.native("label") == label && lv_obj_get_width(label) == 0 && lv_obj_get_height(label) == 0
                && lv_obj_get_style_flex_grow(label, LV_PART_MAIN) == 0, "zero dimensions/weight did not apply on reuse");
            require(lv_obj_get_style_pad_left(root.native("box"), LV_PART_MAIN) == 0 && lv_color_to_u32(lv_obj_get_style_text_color(label, LV_PART_MAIN)) == 0xff000000,
                "zero padding or black text was ignored");
            require(lv_obj_get_scrollbar_mode(root.native("scroll")) == LV_SCROLLBAR_MODE_ON, "forced scrollbar mode did not apply on reuse");
            root.phase = 2; root.redraw(); lv_obj_update_layout(lv_screen_active());
            require(lv_obj_get_width(label) > 0 && lv_obj_get_height(label) > 0 && lv_obj_get_style_bg_opa(label, LV_PART_MAIN) == LV_OPA_TRANSP,
                "omitted dimensions/background did not restore defaults");
            require(lv_obj_get_style_text_font(label, LV_PART_MAIN) == LV_FONT_DEFAULT && lv_obj_get_style_pad_left(root.native("box"), LV_PART_MAIN) == 11,
                "omitted font/padding did not restore defaults");
            require(lv_obj_get_scrollbar_mode(root.native("scroll")) == LV_SCROLLBAR_MODE_OFF
                && lv_obj_get_scroll_dir(root.native("scroll")) == LV_DIR_NONE, "disabled scrolling did not apply on reuse");
            for(int i = 0; i < 20; ++i) { root.phase = i % 3; root.redraw(); }
            auto declaration = find(root, "label"); lv_obj_delete(root.native("label"));
            require(!declaration->ui, "external deletion retained a styled native object");
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "modifier teardown retained native content");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit(); return result;
}

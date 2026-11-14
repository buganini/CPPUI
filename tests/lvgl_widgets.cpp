#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include "cppui/lvgl/divider.hpp"
#include "cppui/lvgl/text.hpp"
#include <memory>
#include <stdexcept>

namespace {
    void require(bool condition, const char * message) { if(!condition) throw std::runtime_error(message); }
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    class Group: public CPPUI::LVGL::View {
        public:
        void content() override { CPPUI::Divider().id("virtual-divider"); }
    };
    class Root: public CPPUI::LVGL::View {
        public:
        std::string text = "<b>literal text</b> and some more words that will wrap across multiple lines";
        std::string html = "<i>HTML</i>\n<span style=\"color:red\">support</span>";
        std::string markdown = "# Heading\n**bold** <i>literal</i>";
        bool show = true, handler = true, selectable = false, margin = true, doubleHandler = false;
        int doubled = 0;
        int value = 1, clicked = 0;
        std::weak_ptr<int> capture;
        std::function<void()> action;
        void content() override {
            for(auto window: CPPUI::Window()) if(show) for(auto column: CPPUI::VBox().id("column")) {
                auto & label = CPPUI::Label("Click me").id("click");
                if(handler) {
                    auto token = std::make_shared<int>(value);
                    capture = token;
                    label.click([this, token]() { clicked = *token; auto invoke = action; if(invoke) invoke(); });
                }
                auto & plain = CPPUI::Label("plain", selectable).id("plain");
                if(margin) plain.layout(CPPUI::layout::margin(1, 4));
                if(doubleHandler) plain.dblclick([this]() { ++doubled; });
                CPPUI::Label("narrow").id("narrow").layout(CPPUI::layout::width(60));
                CPPUI::Divider().id("horizontal");
                for(auto row: CPPUI::HBox().id("row")) {
                    CPPUI::Label("left");
                    CPPUI::SubView<Group>();
                    CPPUI::Label("right");
                }
                CPPUI::Text(text, selectable).id("text");
                CPPUI::Html(html).id("html");
                CPPUI::MarkDown(markdown).id("markdown");
            }
        }
        lv_obj_t * object(const char * id) { return static_cast<lv_obj_t *>(find(*this, id)->ui); }
    };
    void count(lv_event_t * event) { ++*static_cast<int *>(lv_event_get_user_data(event)); }
}
int main() {
    lv_init();
    lv_display_create(480, 320);
    int result = 0;
    try {
        {
            Root root;
            root.redraw();
            auto text = root.object("text"), click = root.object("click"), divider = root.object("horizontal");
            lv_obj_set_width(text, 120);
            lv_obj_update_layout(lv_screen_active());
            require(root.text == lv_label_get_text(text), "Text interpreted markup or lost its owned string");
            require(lv_obj_get_height(text) > 2 * lv_font_get_line_height(LV_FONT_DEFAULT), "Text did not wrap");
            auto html = root.object("html"), markdown = root.object("markdown");
            require(root.html == lv_label_get_text(html) && root.markdown == lv_label_get_text(markdown),
                "HTML or Markdown fallback changed the literal source");
            require(lv_obj_get_height(divider) == 1 && lv_obj_get_width(divider) > 1, "column divider is not horizontal");
            auto vertical = root.object("virtual-divider");
            require(lv_obj_get_width(vertical) == 1 && lv_obj_get_height(vertical) > 1, "virtual row divider is not vertical");
            int external = 0, destroyed = 0;
            lv_obj_add_event_cb(click, count, LV_EVENT_CLICKED, &external);
            lv_obj_add_event_cb(divider, count, LV_EVENT_DELETE, &destroyed);
            auto capture = root.capture;
            root.value = 2;
            root.text = "short";
            root.html = "<b>updated</b>";
            root.markdown = "* updated";
            root.redraw();
            lv_obj_update_layout(lv_screen_active());
            require(root.object("text") == text && root.object("horizontal") == divider, "widgets were not reused");
            require(std::string(lv_label_get_text(text)) == "short" && lv_obj_get_height(text) == lv_font_get_line_height(LV_FONT_DEFAULT), "wrapped text did not shrink");
            require(root.object("html") == html && root.object("markdown") == markdown
                && root.html == lv_label_get_text(html) && root.markdown == lv_label_get_text(markdown),
                "HTML or Markdown fallback update lost native identity or raw source");
            require(capture.expired(), "label retained retired callback");
            lv_obj_send_event(click, LV_EVENT_CLICKED, nullptr);CPPUI::LVGL::dispatch::flush();
            require(root.clicked == 2 && external == 1, "label click used old data or removed external listener");
            capture = root.capture;
            root.handler = false;
            root.redraw();
            lv_obj_send_event(click, LV_EVENT_CLICKED, nullptr);CPPUI::LVGL::dispatch::flush();
            require(capture.expired() && root.clicked == 2 && external == 2, "omitted label callback was not reset");
            root.handler = true;
            root.action = [&root]() { root.show = false; root.redraw(); };
            root.redraw();
            capture = root.capture;
            lv_obj_send_event(click, LV_EVENT_CLICKED, nullptr);CPPUI::LVGL::dispatch::flush();
            require(capture.expired() && destroyed == 1 && !find(root, "click"), "label self-removal leaked callbacks or widgets");
            root.action = {};
            root.show = true;
            root.redraw();
            auto column = root.object("column");
            require(lv_obj_has_flag(root.object("click"), LV_OBJ_FLAG_CLICKABLE), "label with a click handler is not clickable");
            require(!lv_obj_has_flag(root.object("plain"), LV_OBJ_FLAG_CLICKABLE)
                && !lv_obj_has_flag(root.object("text"), LV_OBJ_FLAG_CLICKABLE), "label without a click handler intercepts input");
            require(lv_label_get_long_mode(root.object("plain")) == LV_LABEL_LONG_MODE_CLIP
                && lv_label_get_long_mode(root.object("narrow")) == LV_LABEL_LONG_MODE_WRAP, "label wrapping ignored its declared width");
            require(lv_obj_get_style_margin_left(root.object("plain"), LV_PART_MAIN) == 4
                && lv_obj_get_style_margin_top(root.object("plain"), LV_PART_MAIN) == 1, "box child margin was not applied");
            root.handler = false;
            root.redraw();
            require(!lv_obj_has_flag(root.object("click"), LV_OBJ_FLAG_CLICKABLE), "removed label handler kept input");
            root.handler = true;
            root.margin = false;
            root.redraw();
            require(lv_obj_get_style_margin_left(root.object("plain"), LV_PART_MAIN) == 0, "omitted margin was not reset");
            {
                // Like PUI, every LVGL widget dispatches double-clicks after the event.
                auto plain = root.object("plain");
                root.doubled = 0; root.doubleHandler = true; root.redraw();
                require(lv_obj_has_flag(plain, LV_OBJ_FLAG_CLICKABLE), "double-click handler did not make the label clickable");
                lv_obj_send_event(plain, LV_EVENT_DOUBLE_CLICKED, nullptr);
                require(root.doubled == 0, "double-click callback ran inside the native event");
                CPPUI::LVGL::dispatch::flush();
                require(root.doubled == 1, "double-click callback was not dispatched");
                root.doubleHandler = false; root.redraw();
                lv_obj_send_event(plain, LV_EVENT_DOUBLE_CLICKED, nullptr);CPPUI::LVGL::dispatch::flush();
                require(root.doubled == 1 && !lv_obj_has_flag(plain, LV_OBJ_FLAG_CLICKABLE), "omitted double-click handler kept dispatch or input");
            }
            root.selectable = true;
            root.redraw();
            require(root.object("column") == column, "selectable text remounted the mounted UI");
            root.selectable = false;
            for(int i = 0; i != 20; ++i) { root.show = !root.show; root.redraw(); }
        }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "widget teardown retained native objects");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    lv_deinit();
    return result;
}

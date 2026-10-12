#ifndef CPPUI_COOKBOOK_LVGL_PRESENTATION
#define CPPUI_COOKBOOK_LVGL_PRESENTATION

// Same cookbook composition as Qt; native details stay in the selected backend.
namespace Cookbook { namespace Presentation {
    struct Item {
        int weight;
        bool fixedSize = false;
        explicit Item(int weight = 0): weight(weight) {}
        virtual ~Item() = default;
    };
    class BoxNode: public CPPUI::LVGL::Node, public Item {
        bool horizontal;
        public:
        BoxNode(bool horizontal, int weight): Item(weight), horizontal(horizontal) {}
        void update(CPPUI::LVGL::Node * prev) override {
            ui = prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto box = static_cast<lv_obj_t *>(ui);
            lv_obj_set_flex_flow(box, horizontal ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN);
            lv_obj_set_flex_align(box, LV_FLEX_ALIGN_START,
                                  horizontal ? LV_FLEX_ALIGN_CENTER : LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
            lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(box, 0, 0);
            lv_obj_set_style_radius(box, 0, 0);
            lv_obj_set_style_pad_all(box, dynamic_cast<CPPUI::LVGL::Window *>(dom_parent) ? 11 : 0, 0);
            lv_obj_set_style_pad_row(box, 6, 0);
            lv_obj_set_style_pad_column(box, 6, 0);
            lv_obj_set_size(box, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            if(dynamic_cast<CPPUI::LVGL::Window *>(dom_parent)) lv_obj_set_size(box, lv_pct(100), lv_pct(100));
            lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
        }
        void postSync() override {
            for(auto child: *children) {
                auto object = static_cast<lv_obj_t *>(child->outer());
                auto item = dynamic_cast<Item *>(child);
                int grow = item ? item->weight : 0;
                lv_obj_set_flex_grow(object, grow);
                if(item && item->fixedSize) continue;
                if(horizontal) lv_obj_set_height(object, dynamic_cast<BoxNode *>(child) ? lv_pct(100) : LV_SIZE_CONTENT);
                else lv_obj_set_width(object, lv_pct(100));
            }
        }
    };
    // Separate types preserve the orientation when reconciling anonymous boxes.
    class HBoxNode: public BoxNode { public: explicit HBoxNode(int weight): BoxNode(true, weight) {} };
    class VBoxNode: public BoxNode { public: explicit VBoxNode(int weight): BoxNode(false, weight) {} };
    inline HBoxNode & HBox(int weight = 0) { return *new HBoxNode(weight); }
    inline VBoxNode & VBox(int weight = 0) { return *new VBoxNode(weight); }
    class SidebarNode: public VBoxNode {
        public:
        SidebarNode(): VBoxNode(0) {}
        void update(CPPUI::LVGL::Node * prev) override {
            VBoxNode::update(prev);
            lv_obj_set_width(static_cast<lv_obj_t *>(ui), 200);
        }
    };
    inline SidebarNode & Sidebar() { return *new SidebarNode(); }
    inline CPPUI::LVGL::Window & Window() { return CPPUI::Window(); }

    class ScrollNode: public CPPUI::LVGL::Node, public Item {
        public:
        ScrollNode(): Item(1) {}
        void update(CPPUI::LVGL::Node * prev) override {
            ui = prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_size(object, 190, 0);
            lv_obj_set_style_pad_all(object, 11, 0);
            lv_obj_set_style_radius(object, 0, 0);
            lv_obj_set_scroll_dir(object, LV_DIR_VER);
            lv_obj_set_scrollbar_mode(object, LV_SCROLLBAR_MODE_AUTO);
        }
        void addChild(int idx, CPPUI::LVGL::Node * child) override {
            CPPUI::LVGL::Node::addChild(idx, child);
            lv_obj_set_width(static_cast<lv_obj_t *>(child->outer()), lv_pct(100));
        }
        void postSync() override {
            for(auto child: *children) {
                lv_obj_set_width(static_cast<lv_obj_t *>(child->outer()), lv_pct(100));
            }
        }
    };
    inline ScrollNode & Scroll() { return *new ScrollNode(); }

    class PageLinkNode: public CPPUI::LVGL::Label {
        static void clicked(lv_event_t * event) {
            auto node = static_cast<PageLinkNode *>(lv_event_get_user_data(event));
            auto invoke = node->onClicked;
            invoke();
        }
        public:
        explicit PageLinkNode(const std::string & text): Label(text) {}
        void update(CPPUI::LVGL::Node * prev) override {
            CPPUI::LVGL::Label::update(prev);
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_add_flag(object, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_remove_event_cb(object, clicked);
            lv_obj_add_event_cb(object, clicked, LV_EVENT_CLICKED, this);
        }
        void destroy(bool direct) override {
            if(ui) lv_obj_remove_event_cb(static_cast<lv_obj_t *>(ui), clicked);
            CPPUI::LVGL::Label::destroy(direct);
        }
    };
    inline PageLinkNode & PageLink(const std::string & text) { return *new PageLinkNode(text); }

    class SpacerNode: public CPPUI::LVGL::Spacer, public Item {
        public:
        SpacerNode(): Item(1) {}
    };
    inline SpacerNode & Spacer() { return *new SpacerNode(); }
    class DividerNode: public CPPUI::LVGL::Node {
        public:
        void update(CPPUI::LVGL::Node * prev) override {
            ui = prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_remove_style_all(object);
            lv_obj_set_height(object, 1);
            lv_obj_set_style_bg_color(object, lv_color_hex(0x808080), 0);
            lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
        }
    };
    inline DividerNode & Divider() { return *new DividerNode(); }

    class LabelNode: public CPPUI::LVGL::Label, public Item {
        bool styled;
        public:
        LabelNode(const std::string & text, int weight, bool styled): Label(text), Item(weight), styled(styled) {}
        void update(CPPUI::LVGL::Node * prev) override {
            CPPUI::LVGL::Label::update(prev);
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_style_text_font(object, styled ? &lv_font_montserrat_28 : LV_FONT_DEFAULT, 0);
            if(styled) {
                lv_obj_set_style_text_color(object, lv_color_hex(0xffff00), 0);
                lv_obj_set_style_bg_color(object, lv_color_hex(0xff0000), 0);
                lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
            } else {
                lv_obj_remove_local_style_prop(object, LV_STYLE_TEXT_COLOR, 0);
                lv_obj_remove_local_style_prop(object, LV_STYLE_BG_COLOR, 0);
                lv_obj_remove_local_style_prop(object, LV_STYLE_BG_OPA, 0);
            }
        }
    };
    inline LabelNode & Label(const std::string & text, int weight = 0) { return *new LabelNode(text, weight, false); }
    inline LabelNode & StyledLabel() { return *new LabelNode("Big Yellow", 0, true); }
    class StyledButtonNode: public CPPUI::LVGL::Button {
        public:
        StyledButtonNode(): Button("Styled") {}
        void update(CPPUI::LVGL::Node * prev) override {
            CPPUI::LVGL::Button::update(prev);
            lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
            lv_obj_set_style_text_color(label, lv_color_hex(0xff0000), 0);
        }
    };
    inline StyledButtonNode & StyledButton() { return *new StyledButtonNode(); }

    class CanvasNode: public CPPUI::LVGL::Canvas, public Item {
        public:
        explicit CanvasNode(Painter painter): CPPUI::LVGL::Canvas(std::move(painter)) { fixedSize = true; }
        void update(CPPUI::LVGL::Node * prev) override {
            CPPUI::LVGL::Canvas::update(prev);
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_size(object, 100, 100);
            lv_obj_set_style_bg_color(object, lv_color_hex(0x555555), 0);
            lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
        }
    };
    inline CanvasNode & Canvas(CPPUI::LVGL::Canvas::Painter painter) {
        return *new CanvasNode(std::move(painter));
    }
}}
#endif

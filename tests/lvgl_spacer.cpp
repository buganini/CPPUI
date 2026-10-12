#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#include "cppui/lvgl/label.hpp"
#include <stdexcept>

namespace {
    void require(bool condition, const char * message) {
        if(!condition) throw std::runtime_error(message);
    }
    CPPUI::Node * find(CPPUI::Node & root, const std::string & id) {
        if(root.has_id && root._id == id) return &root;
        for(auto child: *root.children) if(auto match = find(*child, id)) return match;
        return nullptr;
    }
    class GapView: public CPPUI::LVGL::View {
        public:
        void content() override { CPPUI::Spacer().id("gap"); }
    };
    class Root: public CPPUI::Application {
        void items() {
            CPPUI::Label(text).id("first");
            if(gap) {
                if(nested) (new GapView())->id("virtual");
                else CPPUI::Spacer().id("gap");
            }
            if(secondGap) CPPUI::Spacer().id("gap2");
            CPPUI::Label("End").id("last");
        }
        public:
        using Application::Application;
        bool horizontal = true, gap = true, secondGap = false, nested = false, box = true;
        std::string text = "Start";
        void content() override {
            for(auto window: CPPUI::Window()) if(box) {
                if(horizontal) { for(auto row: CPPUI::HBox().id("box")) items(); }
                else { for(auto column: CPPUI::VBox().id("box")) items(); }
            }
        }
        lv_obj_t * native(const char * id) { return static_cast<lv_obj_t *>(find(*this, id)->ui); }
    };
    int extent(lv_obj_t * object, bool horizontal) {
        return horizontal ? lv_obj_get_width(object) : lv_obj_get_height(object);
    }
    void check(Root & root) {
        lv_obj_update_layout(lv_screen_active());
        auto box = root.native("box"), gap = root.native("gap"), last = root.native("last");
        const int available = root.horizontal ? lv_obj_get_content_width(box) : lv_obj_get_content_height(box);
        require(extent(gap, root.horizontal) > available/2, "spacer failed to fill the remaining main-axis space");
        lv_area_t content, bounds;
        lv_obj_get_content_coords(box, &content);
        lv_obj_get_coords(last, &bounds);
        require(root.horizontal ? bounds.x2 == content.x2 : bounds.y2 == content.y2,
                "spacer did not push the trailing control to the content edge");
        require(!lv_obj_is_clickable(gap) && !lv_obj_is_scrollable(gap), "spacer intercepts input or scrolling");
        require(lv_obj_get_style_bg_opa(gap, LV_PART_MAIN) == LV_OPA_TRANSP
            && lv_obj_get_style_border_width(gap, LV_PART_MAIN) == 0, "spacer inherited a visible decoration");
        auto node = find(root, "gap");
        require(node->is_terminal && !node->is_movable, "spacer declaration differs from PUI's terminal/non-movable contract");
    }
    void geometryAndLifetime(int argc, char ** argv) {
        Root root(argc, argv);
        root.redraw(); check(root);
        auto original = root.native("gap");
        const int initial = extent(original, true);
        root.text = "A longer leading label";
        root.redraw(); check(root);
        require(root.native("gap") == original && extent(original,true) < initial,
                "retained spacer failed to resize or replaced its native object");
        for(int i=0; i<20; ++i) { root.redraw(); check(root); }
        lv_display_set_resolution(lv_display_get_default(), 800, 480);
        check(root);
        require(extent(original,true) > initial, "display resize failed to grow the spacer");
        root.secondGap = true; root.redraw();
        lv_obj_update_layout(lv_screen_active());
        const int a = extent(root.native("gap"), true), b = extent(root.native("gap2"), true);
        require(a > 0 && b > 0 && a-b >= -1 && a-b <= 1, "multiple spacers did not divide spare space equally");
        root.secondGap = false; root.horizontal = false;
        root.redraw(); check(root);
        root.nested = true; root.redraw(); check(root);
        auto view = static_cast<GapView *>(find(root,"virtual"));
        auto nestedNative = root.native("gap");
        view->redraw(); check(root);
        require(root.native("gap") == nestedNative, "virtual-view redraw lost its spacer");
        for(int i=0; i<10; ++i) {
            root.gap = false; root.redraw();
            require(lv_obj_get_child_count(root.native("box")) == 2, "spacer removal left a native child");
            root.gap = true; root.redraw(); check(root);
        }
        root.box = false; root.redraw();
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "parent removal retained native spacer content");
        root.box = true; root.nested = false; root.redraw(); check(root);
        // Only the current declaration may observe native deletion after repeated redraws.
        auto current = find(root,"gap");
        lv_obj_delete(root.native("gap"));
        require(current->ui == nullptr, "native deletion left a stale spacer observer");
        // Root teardown must also tolerate an already-deleted spacer.
    }
}
int main(int argc, char ** argv) {
    lv_init();
    lv_display_create(640,360);
    int result = 0;
    try {
        geometryAndLifetime(argc,argv);
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "application teardown retained native content");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    lv_deinit();
    return result;
}

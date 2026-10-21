#include "cppui/lvgl/scroll.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/label.hpp"
#include <stdexcept>

namespace {
    void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
    void settle() {lv_obj_update_layout(lv_screen_active());}
    void animate() {
        for(int i=0;i<60;++i) {lv_tick_inc(10);lv_timer_handler();settle();}
    }
    class Root: public CPPUI::LVGL::View {
    public:
        int width=800,height=1000,viewportWidth=300,viewportHeight=200;
        void content() override {
            for(auto window:CPPUI::Window())
                for(auto scroll:CPPUI::Scroll(true,true).layout(viewportWidth,viewportHeight))
                    CPPUI::Label("scroll content").layout(width,height);
        }
        lv_obj_t * native() {return static_cast<lv_obj_t *>(children->front()->children->front()->outer());}
        void sync() {redraw();settle();}
    };
    void resizing() {
        Root root;root.sync();
        auto scroll=root.native();
        auto content=lv_obj_get_child(scroll,0);
        lv_obj_scroll_to(scroll,150,250,LV_ANIM_ON);animate();
        require(lv_obj_get_scroll_x(scroll)==150 && lv_obj_get_scroll_y(scroll)==250,
                "ordinary scroll animations must reach their destination");
        root.width=900;root.height=1100;root.sync();animate();
        require(root.native()==scroll && lv_obj_get_child(scroll,0)==content
                && lv_obj_get_scroll_x(scroll)==150 && lv_obj_get_scroll_y(scroll)==250,
                "content resizing must preserve native objects and valid scroll offsets");

        // Like focus scrolling before a grid breakpoint, this animation targets
        // content that will become smaller before the animation has finished.
        lv_obj_scroll_to(scroll,500,900,LV_ANIM_ON);
        lv_tick_inc(50);lv_timer_handler();
        root.width=100;root.height=80;root.sync();animate();
        require(lv_obj_get_scroll_x(scroll)==0 && lv_obj_get_scroll_y(scroll)==0
                && lv_obj_is_visible(content),
                "a stale animation scrolled beyond resized content");

        root.width=400;root.height=300;root.sync();
        lv_obj_scroll_to(scroll,100,100,LV_ANIM_ON);
        lv_tick_inc(50);lv_timer_handler();
        root.viewportWidth=500;root.viewportHeight=350;root.sync();animate();
        require(lv_obj_get_scroll_x(scroll)==0 && lv_obj_get_scroll_y(scroll)==0,
                "a stale animation scrolled beyond an enlarged viewport");

        root.width=800;root.height=1000;root.sync();
        lv_obj_scroll_to(scroll,150,250,LV_ANIM_ON);animate();
        require(lv_obj_get_scroll_x(scroll)==150 && lv_obj_get_scroll_y(scroll)==250,
                "scroll animation stopped working after a resize");
    }
}
int main() {
    lv_init();lv_display_create(640,480);int result=0;
    try {resizing();require(lv_obj_get_child_count(lv_screen_active())==0,"scroll teardown leaked native content");}
    catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
    lv_deinit();return result;
}

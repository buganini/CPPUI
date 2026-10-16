#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/canvas.hpp"
#include <stdexcept>

namespace {
    void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
    struct Driver {
        lv_indev_data_t packet{};
        lv_indev_t * device;
        explicit Driver(lv_indev_type_t type) {
            device=lv_indev_create(); lv_indev_set_type(device,type);
            lv_indev_set_user_data(device,this);
            lv_indev_set_read_cb(device,[](lv_indev_t * device,lv_indev_data_t * data) {
                auto driver=static_cast<Driver *>(lv_indev_get_user_data(device));
                *data=driver->packet; driver->packet.enc_diff=0;
            });
        }
        ~Driver() { lv_indev_delete(device); }
        void read(int x,int y,int steps=0) { packet.point={x,y}; packet.enc_diff=steps; lv_indev_read(device); }
    };
    class Root:public CPPUI::Application {
    public:
        using Application::Application;
        bool show=true,callbacks=true,remove=false,fail=false;
        int version=1,moves=0,leaves=0,wheels=0;
        CPPUI::CanvasPointerEvent last;
        std::shared_ptr<int> token=std::make_shared<int>(0);
        void content() override {
            for(auto window:CPPUI::Window()) if(show) {
                auto & canvas=CPPUI::Canvas([](CPPUI::CanvasPainter &){});
                if(callbacks) {
                    const int current=version; auto capture=token;
                    canvas.mousemove([this,current,capture](const CPPUI::CanvasPointerEvent & event){moves+=current;last=event;})
                        .mouseleave([this,current,capture](){leaves+=current;})
                        .wheel([this,current,capture](const CPPUI::CanvasPointerEvent & event){
                            wheels+=current;last=event;
                            if(fail) throw std::runtime_error("expected wheel failure");
                            if(remove) { show=false;redraw(); }
                        });
                }
                canvas.layout(120,80);
            }
        }
        lv_obj_t * native() { return static_cast<lv_obj_t *>(children->front()->children->front()->ui); }
    };
}
int main(int argc,char **argv) {
    lv_init(); auto display=lv_display_create(240,160);
    int result=0;
    try {
        Driver pointer(LV_INDEV_TYPE_POINTER),encoder(LV_INDEV_TYPE_ENCODER);
        Root root(argc,argv); root.redraw();auto canvas=root.native();lv_obj_set_pos(canvas,20,30);
        const auto base=lv_indev_get_event_count(pointer.device);
        for(int i=1;i<=3;++i) {
            root.version=i;root.redraw(); require(root.native()==canvas,"input redraw replaced Canvas");
            int before=root.moves;pointer.read(40+i,50);
            require(root.moves==before+i && root.last.x==20+i && root.last.y==20,"move lost local coordinates/current callback");
            pointer.read(40+i,50);require(root.moves==before+i,"stationary pointer repeated move");
            before=root.wheels;pointer.read(40+i,50,-1);
            require(root.wheels==before+i && root.last.angleY==120 && root.last.pixelY==0,"SDL-style pointer wheel failed");
            before=root.wheels;encoder.read(0,0,1);
            require(root.wheels==before+i && root.last.angleY==-120,"X11-style encoder wheel failed");
            before=root.leaves;pointer.read(200,140);require(root.leaves==before+i,"leave callback failed");
            require(lv_indev_get_event_count(pointer.device)==base,"redraw accumulated input observers");
        }
        // A top-layer overlay must intercept Canvas hover/wheel input.
        pointer.read(45,50);auto overlay=lv_obj_create(lv_display_get_layer_top(display));
        lv_obj_set_pos(overlay,20,30);lv_obj_set_size(overlay,120,80);
        int before=root.wheels;pointer.read(45,50,-1);require(root.wheels==before,"wheel passed through overlay");
        lv_obj_delete(overlay);
        pointer.read(45,50);before=root.leaves;
        lv_indev_send_event(pointer.device,LV_EVENT_LEAVE,nullptr);
        require(root.leaves==before+root.version,"native window exit did not leave Canvas");
        before=root.moves;pointer.read(45,50);require(root.moves==before,"stale X11 position restored hover outside window");
        lv_indev_send_event(pointer.device,LV_EVENT_HOVER_OVER,nullptr);pointer.read(45,50);
        require(root.moves==before+root.version,"window entry did not restore hover");
        root.callbacks=false;root.redraw();before=root.wheels;pointer.read(45,50,-1);
        require(root.wheels==before && lv_indev_get_event_count(pointer.device)==base-1,"omission retained input dispatch");
        require(!lv_obj_has_flag(canvas,LV_OBJ_FLAG_CLICKABLE),"Canvas without handlers intercepted input");
        root.callbacks=true;root.redraw();
        { Driver temporary(LV_INDEV_TYPE_POINTER);root.redraw();temporary.read(40,40); }
        root.redraw(); // Device deletion must remove its borrowed observer.
        root.fail=true;pointer.read(45,50,-1);root.fail=false;
        std::weak_ptr<int> token=root.token;root.token.reset();root.remove=true;root.redraw();
        pointer.read(45,50,-1);require(!root.show && token.expired(),"wheel self-removal retained callback/native state");
        require(lv_indev_get_event_count(pointer.device)==base-1,"removal retained an input observer");
        root.show=true;root.remove=false;root.redraw();lv_obj_delete(root.native());
        require(lv_indev_get_event_count(pointer.device)==base-1,"native deletion retained input observer");
    } catch(const std::exception & error) { std::cerr<<error.what()<<'\n';result=1; }
    lv_deinit();return result;
}

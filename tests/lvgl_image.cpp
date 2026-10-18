#include "cppui/lvgl/image.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/layout.hpp"
#define LODEPNG_NO_COMPILE_CPP
#include <src/libs/lodepng/lodepng.h>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>

namespace {
    void require(bool ok,const char * message) {if(!ok) throw std::runtime_error(message);}
    struct Files {
        std::string directory,first,second,broken,jpeg;
        Files() {
            char name[]="/tmp/cppui-lvgl-image-XXXXXX";
            auto created=mkdtemp(name);require(created,"temporary directory failed");directory=created;
            first=directory+"/first.png";second=directory+"/second.png";
            broken=directory+"/broken.png";jpeg=directory+"/truncated.jpg";
        }
        ~Files() {for(auto path:{first,second,broken,jpeg}) unlink(path.c_str());rmdir(directory.c_str());}
    };
    void png(const std::string & path,int w,int h,unsigned rgb,unsigned alpha=255) {
        std::vector<unsigned char> rgba(w*h*4);
        for(int i=0;i<w*h;++i) {rgba[i*4]=rgb>>16;rgba[i*4+1]=rgb>>8;rgba[i*4+2]=rgb;rgba[i*4+3]=alpha;}
        unsigned char * encoded=nullptr;size_t size=0;
        require(lodepng_encode32(&encoded,&size,rgba.data(),w,h)==0,"PNG encode failed");
        std::unique_ptr<unsigned char,decltype(&lv_free)> owner(encoded,lv_free);
        std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<char *>(encoded),size);
        require(bool(file),"PNG fixture write failed");
    }
    class Root: public CPPUI::LVGL::View {
    public:
        std::string path;
        CPPUI::Optional<int> width,height,weight;
        bool show=true,hidden=false,callback=true,row=false;
        int clicked=0;
        std::function<void()> action;
        void content() override {
            for(auto window:CPPUI::Window()) for(auto box:row ? static_cast<CPPUI::Node &>(CPPUI::HBox()) : CPPUI::VBox()) if(show) {
                auto & image=CPPUI::Image(path);
                image.layout(width,height,weight).visible(!hidden);
                if(callback) image.click([this]() {++clicked;auto invoke=action;if(invoke) invoke();});
            }
        }
        CPPUI::Node * node() {return children->front()->children->front()->children->front();}
        lv_obj_t * object() {return static_cast<lv_obj_t *>(node()->ui);}
        const lv_draw_buf_t * pixels() {return static_cast<const lv_draw_buf_t *>(lv_image_get_src(object()));}
        void sync() {redraw();lv_obj_update_layout(lv_screen_active());}
    };
    lv_color32_t pixel(const lv_draw_buf_t * buffer,int x=0,int y=0) {
        auto canvas=lv_canvas_create(lv_screen_active());lv_obj_set_hidden(canvas,true);
        lv_canvas_set_draw_buf(canvas,const_cast<lv_draw_buf_t *>(buffer));
        auto result=lv_canvas_get_px(canvas,x,y);lv_obj_delete(canvas);return result;
    }
    void rendering(Root & root) {
        // Render the retained source through the native image drawing path.
        auto canvas=lv_canvas_create(lv_screen_active());
        auto buffer=lv_draw_buf_create(60,40,LV_COLOR_FORMAT_ARGB8888,LV_STRIDE_AUTO);
        require(buffer,"render buffer allocation failed");
        lv_draw_buf_clear(buffer,nullptr);
        lv_canvas_set_draw_buf(canvas,buffer);lv_canvas_fill_bg(canvas,lv_color_white(),LV_OPA_COVER);
        lv_layer_t layer;lv_canvas_init_layer(canvas,&layer);
        lv_draw_image_dsc_t image;lv_draw_image_dsc_init(&image);image.src=root.pixels();
        image.scale_x=image.scale_y=LV_SCALE_NONE*2;image.pivot={0,0};
        lv_area_t area{0,0,int(root.pixels()->header.w)-1,int(root.pixels()->header.h)-1};
        lv_draw_image(&layer,&image,&area);lv_canvas_finish_layer(canvas,&layer);
        const auto value=lv_canvas_get_px(canvas,10,10);
        require(value.red>245 && value.green>115 && value.green<140 && value.blue>115 && value.blue<140,
            "PNG alpha or scaled native image drawing is incorrect");
        lv_obj_delete(canvas);lv_draw_buf_destroy(buffer);
    }
    void geometry(Files & files) {
        Root root;root.path=files.first;root.row=true;root.sync();
        auto object=root.object();auto source=root.pixels();
        require(source && source->header.w==20 && source->header.h==10,"PNG size not decoded");
        require(lv_obj_get_width(object)==20 && lv_obj_get_height(object)==10,"natural image size is incorrect");
        auto value=pixel(source);require(value.red==255 && value.alpha==128,"PNG alpha was lost");
        rendering(root);
        for(int i=0;i<12;++i) root.sync();
        require(root.object()==object && root.pixels()==source,"unchanged image decoded or remounted");
        root.width=80;root.sync();
        require(lv_obj_get_width(object)==80 && lv_obj_get_height(object)==40,"width-only aspect scaling failed");
        root.width.reset();root.height=30;root.sync();
        require(lv_obj_get_width(object)==60 && lv_obj_get_height(object)==30,"height-only aspect scaling failed");
        root.width=80;root.height=80;root.sync();
        require(lv_image_get_inner_align(object)==LV_IMAGE_ALIGN_CONTAIN && lv_image_get_scale_x(object)==lv_image_get_scale_y(object),"two dimensions distorted aspect ratio");
        root.weight=1;root.sync();require(lv_image_get_inner_align(object)==LV_IMAGE_ALIGN_STRETCH,"positive weight did not enable stretching");
        root.weight.reset();root.width.reset();root.height.reset();root.sync();
        require(root.pixels()==source && lv_obj_get_width(object)==20 && lv_obj_get_height(object)==10
            && lv_image_get_inner_align(object)==LV_IMAGE_ALIGN_CONTAIN,"omitted dimensions/weight did not restore natural size");
        root.hidden=true;root.sync();require(lv_obj_is_hidden(object) && root.pixels()==source,"hide lost image source");
        root.hidden=false;root.path=files.second;root.sync();
        require(root.object()==object && root.pixels()->header.w==10 && root.pixels()->header.h==30,"source replacement lost native reuse or size");
        const auto before=root.pixels();
        png(files.second,10,30,0x00ff00);
        struct stat info{};require(stat(files.second.c_str(),&info)==0,"fixture stat failed");
        timespec times[2]={{info.st_atime,0},{info.st_mtime+2,123456}};
        require(utimensat(AT_FDCWD,files.second.c_str(),times,0)==0,"fixture timestamp update failed");
        root.sync();require(root.pixels()!=before && pixel(root.pixels()).green==255,"same-path replacement retained stale pixels");
        root.path=files.directory+"/missing.png";
        bool failed=false;try {root.sync();} catch(const std::runtime_error &) {failed=true;}
        require(failed && root.object()==object && pixel(root.pixels()).green==255,"missing path discarded mounted image");
        root.path=std::string(CPPUI_TEST_ASSET_DIR)+"/windmill.jpg";root.sync();
        require(root.pixels()->header.w==400 && root.pixels()->header.h==339,"JPEG tiles were not fully decoded");
        // Exercise the final partial MCU as well as the first tile.
        auto first=pixel(root.pixels()),last=pixel(root.pixels(),399,338);
        require(first.alpha==255 && last.alpha==255 && (first.red!=last.red || first.green!=last.green),"JPEG edge tiles contain no image data");
    }
    void lifecycle(Files & files) {
        Root root;root.path=files.first;root.sync();auto object=root.object();
        lv_obj_send_event(object,LV_EVENT_CLICKED,nullptr);require(root.clicked==1,"image click missing");
        root.callback=false;root.sync();lv_obj_send_event(object,LV_EVENT_CLICKED,nullptr);
        require(root.clicked==1,"omitted click callback retained");
        root.callback=true;root.action=[&]() {root.sync();root.show=false;root.sync();};root.sync();
        lv_obj_send_event(object,LV_EVENT_CLICKED,nullptr);require(root.clicked==2,"redraw/removal during click failed");
        root.show=true;root.action={};root.sync();
        auto declaration=root.node();lv_obj_delete(root.object());require(!declaration->ui,"external native deletion left stale image");
        root.sync();require(root.object() && root.pixels(),"native image was not recreated after external deletion");
        // Declarations are deleted before a native parent subtree; the source
        // must survive until each native image has finished being deleted.
        for(int i=0;i<12;++i) {root.show=false;root.sync();root.show=true;root.sync();}
    }
    void failures(Files & files) {
        std::ofstream bad(files.broken,std::ios::binary);bad<<"not an image";bad.close();
        std::ifstream input(std::string(CPPUI_TEST_ASSET_DIR)+"/windmill.jpg",std::ios::binary);
        std::vector<char> contents((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());
        std::ofstream truncated(files.jpeg,std::ios::binary);truncated.write(contents.data(),contents.size()/2);truncated.close();
        for(auto path:{files.broken,files.jpeg}) {
            Root root;root.path=files.first;root.sync();auto object=root.object();auto pixels=root.pixels();
            root.path=path;bool failed=false;
            try {root.sync();} catch(const std::runtime_error &) {failed=true;}
            require(failed,"invalid image did not report decode failure");
            require(root.object()==object && root.pixels()==pixels,"bad image replacement discarded mounted pixels");
        }
    }
}
int main() {
    lv_init();lv_display_create(640,480);
    // Also exercise callers that enable the optional native decoder caches.
    lv_image_cache_resize(1024*1024,true);lv_image_header_cache_resize(8,true);
    int result=0;
    try {
        Files files;png(files.first,20,10,0xff0000,128);png(files.second,10,30,0x0000ff);
        geometry(files);lifecycle(files);failures(files);
        require(lv_obj_get_child_count(lv_screen_active())==0,"image teardown retained native objects");
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';result=1;}
    lv_deinit();return result;
}

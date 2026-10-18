#ifndef CPPUI_LVGL_IMAGE
#define CPPUI_LVGL_IMAGE

#include "base.hpp"
#include <src/image/lv_image_decoder_private.h>
#include <src/misc/cache/instance/lv_image_cache.h>
#include <src/misc/cache/instance/lv_image_header_cache.h>
#include <algorithm>
#include <sys/stat.h>

namespace CPPUI { namespace LVGL {
class Image: public Node {
    using Buffer = std::shared_ptr<lv_draw_buf_t>;
    struct Source {
        std::string path;
        struct stat stamp{};
        Buffer pixels;
        ~Source() {if(pixels) lv_image_cache_drop(pixels.get());}
        bool matches(const std::string & value,const struct stat & info) const {
            return path==value && stamp.st_dev==info.st_dev && stamp.st_ino==info.st_ino
                && stamp.st_size==info.st_size && stamp.st_mtime==info.st_mtime
#if defined(__linux__)
                && stamp.st_mtim.tv_nsec==info.st_mtim.tv_nsec
#elif defined(__APPLE__)
                && stamp.st_mtimespec.tv_nsec==info.st_mtimespec.tv_nsec
#endif
                ;
        }
    };
    struct Native {
        lv_obj_t * object=nullptr;
        std::shared_ptr<Source> source;
        static void deleted(lv_event_t * event) {
            auto holder=static_cast<std::shared_ptr<Native> *>(lv_event_get_user_data(event));
            (*holder)->object=nullptr;
            (*holder)->source.reset();
            delete holder;
        }
    };
    std::shared_ptr<Native> native;
    std::shared_ptr<Source> source;
    int naturalWidth=0,naturalHeight=0;

    static Buffer decode(const std::string & path) {
#if LV_USE_FS_STDIO && LV_FS_STDIO_LETTER
        const auto file=std::string(1,LV_FS_STDIO_LETTER)+":"+path;
        // A same-path replacement must not inherit cached dimensions or pixels.
        lv_image_header_cache_drop(file.c_str());
        lv_image_decoder_args_t args{};args.no_cache=true;args.stride_align=true;
        lv_image_decoder_dsc_t decoder{};
        if(lv_image_decoder_open(&decoder,file.c_str(),&args)!=LV_RESULT_OK)
            throw std::runtime_error("Cannot decode image: "+path);
        struct Close {
            lv_image_decoder_dsc_t * decoder;
            ~Close() {lv_image_decoder_close(decoder);}
        } close{&decoder};
        if(!decoder.header.w || !decoder.header.h) throw std::runtime_error("Empty image: "+path);
        Buffer pixels(nullptr,lv_draw_buf_destroy);
        if(decoder.decoded) pixels.reset(lv_draw_buf_dup(decoder.decoded),lv_draw_buf_destroy);
        else {
            // Streaming decoders (including TJpgDec) produce tiles. Assemble one
            // owned source so scaling and redraw never re-read the file.
            pixels.reset(lv_draw_buf_create(decoder.header.w,decoder.header.h,
                static_cast<lv_color_format_t>(decoder.header.cf),LV_STRIDE_AUTO),lv_draw_buf_destroy);
            if(!pixels) throw std::bad_alloc();
            const lv_area_t full{0,0,int32_t(decoder.header.w)-1,int32_t(decoder.header.h)-1};
            lv_area_t area{LV_COORD_MIN,LV_COORD_MIN,LV_COORD_MIN,LV_COORD_MIN};
            std::uint64_t remaining=std::uint64_t(decoder.header.w)*decoder.header.h;
            while(remaining) {
                if(lv_image_decoder_get_area(&decoder,&full,&area)!=LV_RESULT_OK || !decoder.decoded
                   || area.x1<0 || area.y1<0 || area.x2>full.x2 || area.y2>full.y2
                   || area.x1>area.x2 || area.y1>area.y2)
                    throw std::runtime_error("Cannot decode image pixels: "+path);
                const auto count=std::uint64_t(lv_area_get_width(&area))*lv_area_get_height(&area);
                if(count>remaining) throw std::runtime_error("Invalid image decoder area: "+path);
                lv_draw_buf_copy(pixels.get(),&area,decoder.decoded,nullptr);
                remaining-=count;
            }
        }
        if(!pixels) throw std::bad_alloc();
        return pixels;
#else
        throw std::runtime_error("Image(path) requires LV_USE_FS_STDIO and a nonzero LV_FS_STDIO_LETTER: "+path);
#endif
    }
    static std::shared_ptr<Source> load(const std::string & path) {
        struct stat stamp{};
        if(stat(path.c_str(),&stamp)!=0) throw std::runtime_error("Image file does not exist: "+path);
        // Weak entries share decoding across declarations without keeping pixels
        // alive after the last native image. Prune expired keys on every load.
        static std::vector<std::weak_ptr<Source>> cache;
        std::shared_ptr<Source> result;
        for(auto it=cache.begin();it!=cache.end();) {
            if(auto existing=it->lock()) {
                if(existing->matches(path,stamp)) result=existing;
                ++it;
            } else it=cache.erase(it);
        }
        if(result) return result;
        result=std::make_shared<Source>();
        result->path=path;result->stamp=stamp;result->pixels=decode(path);
        cache.push_back(result);
        return result;
    }
protected:
    void nativeEvent(lv_event_t * event) override {
        if(lv_event_get_code(event)==LV_EVENT_GET_SELF_SIZE) {
            auto size=static_cast<lv_point_t *>(lv_event_get_param(event));
            size->x=naturalWidth;size->y=naturalHeight;
        } else Node::nativeEvent(event);
    }
public:
    explicit Image(const std::string & path):source(load(path)) {
        is_terminal=true;clickable=true;
    }
    void update(Node * previous) override {
        auto prev=static_cast<Image *>(previous);
        if(prev && prev->native && prev->native->object) native=prev->native;
        else {
            native=std::make_shared<Native>();
            std::unique_ptr<std::shared_ptr<Native>> holder(new std::shared_ptr<Native>(native));
            native->object=lv_image_create(static_cast<lv_obj_t *>(parent->inner()));
            lv_obj_add_event_cb(native->object,Native::deleted,LV_EVENT_DELETE,holder.release());
        }
        ui=native->object;
        const auto & header=source->pixels->header;
        double ratio=1;
        if(layout_width && *layout_width>0) ratio=double(*layout_width)/header.w;
        if(layout_height && *layout_height>0) ratio=layout_width && *layout_width>0
            ? std::min(ratio,double(*layout_height)/header.h) : double(*layout_height)/header.h;
        naturalWidth=std::max(1,int(header.w*ratio));naturalHeight=std::max(1,int(header.h*ratio));
        if(native->source!=source) {
            lv_image_set_src(native->object,source->pixels.get());
            native->source=source;
        }
        lv_image_set_inner_align(native->object,layout_weight && *layout_weight>0 ? LV_IMAGE_ALIGN_STRETCH : LV_IMAGE_ALIGN_CONTAIN);
        lv_image_set_antialias(native->object,true);
        lv_obj_set_size(native->object,LV_SIZE_CONTENT,LV_SIZE_CONTENT);
    }
    void postUpdate() override {
        // Install/transfer the declaration observer before measuring scaled size.
        if(ui) lv_obj_refresh_self_size(static_cast<lv_obj_t *>(ui));
        Node::postUpdate();
    }
    void destroy(bool direct) override {
        Node::destroy(direct);
        // Indirect deletion leaves the pixels with the native object's holder.
        native.reset();
        source.reset();
    }
};
}
inline LVGL::Image & Image(const std::string & path) {return *new LVGL::Image(path);}
}
#endif

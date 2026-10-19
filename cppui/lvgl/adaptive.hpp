#ifndef CPPUI_LVGL_ADAPTIVE
#define CPPUI_LVGL_ADAPTIVE
#include "../view.hpp"
#include "lvgl.h"

namespace CPPUI { namespace LVGL {
    class SizeObserver: public detail::SizeObserver {
        lv_obj_t * object;
        std::weak_ptr<detail::SizeState> state;
        bool pending=false;
        static void measure(void * data) {
            auto self=static_cast<SizeObserver *>(data);self->pending=false;
            if(!self->object) return;
            lv_obj_update_layout(self->object);
            const int width=lv_obj_get_content_width(self->object),height=lv_obj_get_content_height(self->object);
            auto current=self->state.lock();
            if(current) current->publish(width,height);
        }
        static void event(lv_event_t * event) {
            auto self=static_cast<SizeObserver *>(lv_event_get_user_data(event));
            auto code=lv_event_get_code(event);
            if(code==LV_EVENT_DELETE) self->object=nullptr;
            else if(code==LV_EVENT_SIZE_CHANGED || code==LV_EVENT_STYLE_CHANGED) self->refresh();
        }
    public:
        SizeObserver(lv_obj_t * object,std::shared_ptr<detail::SizeState> state):object(object),state(state) {
            lv_obj_add_event_cb(object,event,LV_EVENT_ALL,this);
        }
        ~SizeObserver() override {
            if(pending) lv_async_call_cancel(measure,this);
            if(object) lv_obj_remove_event_cb_with_user_data(object,event,this);
        }
        void refresh() override {
            if(pending || !object) return;
            pending=true;lv_async_call(measure,this);
        }
    };
}}
#endif

#ifndef CPPUI_LVGL_SPLITTER
#define CPPUI_LVGL_SPLITTER

#include "base.hpp"

namespace CPPUI { namespace LVGL {
    class Splitter: public Layout {
        // Native callbacks outlive individual declarations. Keep their state
        // stable through redraws, without retaining a declaration pointer.
        struct Native: std::enable_shared_from_this<Native> {
            struct Pane {
                lv_obj_t * object;
                Insets margin;
                int32_t extent;
                double fraction;
            };
            lv_obj_t * host=nullptr, *dragging=nullptr;
            std::vector<Pane> panes;
            std::vector<lv_obj_t *> handles,watched;
            bool vertical=false,changed=true,arranging=false,syncing=false;
            int32_t available=-1,startPointer=0,startFirst=0,startSecond=0;

            ~Native() { detach(); }
            void detach() {
                if(!host) return;
                lv_obj_remove_event_cb_with_user_data(host,event,this);
                for(auto object:watched) lv_obj_remove_event_cb_with_user_data(object,event,this);
                for(auto handle:handles) if(handle) lv_obj_remove_event_cb_with_user_data(handle,event,this);
                host=nullptr;
            }
            static void event(lv_event_t * event) {
                auto self=static_cast<Native *>(lv_event_get_user_data(event))->shared_from_this();
                auto target=lv_event_get_target_obj(event);
                const auto code=lv_event_get_code(event);
                if(code==LV_EVENT_DELETE) {
                    if(target==self->host) {self->host=nullptr;self->dragging=nullptr;}
                    else {
                        for(auto & pane:self->panes) if(pane.object==target) pane.object=nullptr;
                        for(auto & handle:self->handles) if(handle==target) handle=nullptr;
                        auto & watched=self->watched;
                        watched.erase(std::remove(watched.begin(),watched.end(),target),watched.end());
                        if(self->dragging==target) self->dragging=nullptr;
                    }
                    return;
                }
                if(!self->host) return;
                if(target==self->host) {
                    if(code==LV_EVENT_SIZE_CHANGED || code==LV_EVENT_STYLE_CHANGED || code==LV_EVENT_LAYOUT_CHANGED)
                        self->arrange();
                    return;
                }
                if(code==LV_EVENT_RELEASED || code==LV_EVENT_PRESS_LOST) {self->dragging=nullptr;return;}
                if(code!=LV_EVENT_PRESSED && code!=LV_EVENT_PRESSING) return;
                auto found=std::find(self->handles.begin(),self->handles.end(),target);
                if(found==self->handles.end()) return;
                auto input=lv_indev_active();
                if(!input || lv_indev_get_type(input)!=LV_INDEV_TYPE_POINTER) return;
                lv_point_t point;lv_indev_get_point(input,&point);
                const int32_t coordinate=self->vertical ? point.y : point.x;
                const auto index=std::size_t(found-self->handles.begin());
                if(code==LV_EVENT_PRESSED) {
                    self->dragging=target;self->startPointer=coordinate;
                    self->startFirst=self->panes[index].extent;self->startSecond=self->panes[index+1].extent;
                } else if(self->dragging==target) {
                    const int32_t total=self->startFirst+self->startSecond;
                    const int32_t minimum=total>=2 ? 1 : 0;
                    const int32_t first=std::max(minimum,std::min(total-minimum,self->startFirst+coordinate-self->startPointer));
                    self->panes[index].extent=first;self->panes[index+1].extent=total-first;
                    if(self->available>0) for(auto & pane:self->panes)
                        pane.fraction=double(pane.extent)/self->available;
                    self->arrange();
                }
            }
            void arrange() {
                if(!host || arranging || changed || syncing) return;
                arranging=true;
                const int32_t width=std::max<int32_t>(0,lv_obj_get_content_width(host));
                const int32_t height=std::max<int32_t>(0,lv_obj_get_content_height(host));
                const int32_t axis=vertical ? height : width;
                const int32_t count=static_cast<int32_t>(panes.size());
                // Leave space for panes even when the viewport is smaller than
                // the normal 12-pixel drag handles.
                const int32_t gap=count>1 ? std::min<int32_t>(12,axis/(2*count-1)) : 0;
                const int32_t usable=axis-gap*std::max(0,count-1);
                if(available!=usable) {
                    dragging=nullptr;
                    int32_t remaining=usable;
                    const int32_t minimum=usable>=count ? 1 : 0;
                    for(int32_t i=0;i<count;++i) {
                        const int32_t share=int32_t(panes[i].fraction*usable);
                        panes[i].extent=i==count-1 ? remaining
                            : std::max(minimum,std::min(remaining-minimum*(count-i-1),share));
                        remaining-=panes[i].extent;
                    }
                    available=usable;
                }
                int32_t position=0;
                for(std::size_t i=0;i<panes.size() && host;++i) {
                    const auto pane=panes[i];
                    if(pane.object) {
                        const int32_t w=vertical ? width : pane.extent;
                        const int32_t h=vertical ? pane.extent : height;
                        lv_obj_set_pos(pane.object,(vertical ? 0 : position)+pane.margin.left,
                            (vertical ? position : 0)+pane.margin.top);
                        lv_obj_set_size(pane.object,std::max(0,w-pane.margin.left-pane.margin.right),
                            std::max(0,h-pane.margin.top-pane.margin.bottom));
                    }
                    position+=pane.extent;
                    if(i<handles.size() && handles[i]) {
                        lv_obj_set_pos(handles[i],vertical ? 0 : position,vertical ? position : 0);
                        lv_obj_set_size(handles[i],vertical ? width : gap,vertical ? gap : height);
                        position+=gap;
                    }
                }
                arranging=false;
            }
            void reconcile() {
                if(!host) return;
                if(changed) {
                    dragging=nullptr;
                    for(auto handle:handles) if(handle) {
                        lv_obj_remove_event_cb_with_user_data(handle,event,this);
                        lv_obj_delete(handle);
                    }
                    handles.clear();
                    for(std::size_t i=1;i<panes.size();++i) {
                        auto handle=lv_obj_create(host);
                        lv_obj_remove_style_all(handle);
                        lv_obj_set_style_bg_color(handle,lv_palette_main(LV_PALETTE_GREY),0);
                        lv_obj_set_style_bg_opa(handle,LV_OPA_40,0);
                        lv_obj_set_style_bg_opa(handle,LV_OPA_70,LV_STATE_PRESSED);
                        lv_obj_set_scrollable(handle,false);
                        lv_obj_set_press_lock(handle,true);
                        lv_obj_set_click_focusable(handle,false);
                        lv_obj_set_scroll_chain(handle,false);
                        lv_obj_add_event_cb(handle,event,LV_EVENT_ALL,this);
                        handles.push_back(handle);
                    }
                    for(std::size_t i=0;i<panes.size();++i) {
                        if(panes[i].object) lv_obj_move_to_index(panes[i].object,int32_t(2*i));
                        if(i<handles.size()) lv_obj_move_to_index(handles[i],int32_t(2*i+1));
                    }
                    for(auto & pane:panes) pane.fraction=1.0/panes.size();
                    available=-1;changed=false;
                }
                arrange();
            }
        };
        bool vertical;
        std::shared_ptr<Native> native{new Native()};
    public:
        explicit Splitter(bool vertical=false):vertical(vertical) {}
        bool reflowChildren() const override {return true;}
        void update(::CPPUI::Node * previous) override {
            // Base update restores the previous declaration's padding first.
            // Do not rescale panes against that temporary content rectangle.
            if(previous) static_cast<Splitter *>(previous)->native->syncing=true;
            Node::update(previous);
        }
        void update(Node * previous) override {
            auto prev=static_cast<Splitter *>(previous);
            if(prev && prev->ui) native=prev->native;
            native->syncing=true;
            ui=prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object=static_cast<lv_obj_t *>(ui);
            if(!native->host) {
                native->host=object;
                lv_obj_add_event_cb(object,Native::event,LV_EVENT_ALL,native.get());
            }
            if(native->vertical!=vertical) {native->vertical=vertical;native->changed=true;}
            lv_obj_set_style_bg_opa(object,LV_OPA_TRANSP,0);
            lv_obj_set_style_border_width(object,0,0);lv_obj_set_style_radius(object,0,0);
            lv_obj_set_style_pad_all(object,0,0);
            lv_obj_set_scrollable(object,false);
            lv_obj_set_size(object,lv_pct(100),lv_pct(100));
            lv_obj_set_flex_grow(object,1);
        }
        void addChild(int index,Node * child) override {
            if(child->is_outoforder || !child->ui || !ui) return;
            auto object=static_cast<lv_obj_t *>(child->outer());
            native->panes.insert(native->panes.begin()+index,{object,child->layout_margin.valueOr(Insets(0)),0,0});
            if(std::find(native->watched.begin(),native->watched.end(),object)==native->watched.end()) {
                native->watched.push_back(object);
                lv_obj_add_event_cb(object,Native::event,LV_EVENT_DELETE,native.get());
            }
            native->changed=true;
        }
        void removeChild(int index,Node * child) override {
            if(child->is_outoforder || !ui) return;
            native->panes.erase(native->panes.begin()+index);
            native->changed=true;
        }
        void layoutChild(Node * child) override {
            for(auto & pane:native->panes) if(pane.object==child->outer()) pane.margin=child->layout_margin.valueOr(Insets(0));
        }
        void postSync() override {native->syncing=false;native->reconcile();}
        void postUpdate() override {Node::postUpdate();native->syncing=false;native->reconcile();}
        void destroy(bool direct) override {native->detach();Node::destroy(direct);}
    };
}}
namespace CPPUI { inline LVGL::Splitter & Splitter(bool vertical=false) {return *new LVGL::Splitter(vertical);} }
#endif

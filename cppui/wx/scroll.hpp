#ifndef CPPUI_WX_SCROLL
#define CPPUI_WX_SCROLL
#include "base.hpp"
#include "../scroll.hpp"
#include <wx/scrolwin.h>
#include <cmath>
namespace CPPUI { namespace Wx {
class Scroll: public SingleChild {
    struct Axis { bool end=false; double offset=0; Optional<double> request; };
    struct Position { Axis axes[2]; bool pending=false, scheduled=false; };
    std::shared_ptr<Position> position;
    Optional<bool> verticalPolicy, horizontalPolicy;
    Optional<double> requests[2];
    static wxScrollbarVisibility policy(Optional<bool> value) { return !value ? wxSHOW_SB_DEFAULT : *value ? wxSHOW_SB_ALWAYS : wxSHOW_SB_NEVER; }
    Scroll & request(int axis, double value) {
        if(!std::isfinite(value)) throw std::invalid_argument("Scroll offset must be finite");
        requests[axis] = value;
        if(position) { set(axis,value); restore(); queueRestore(); }
        return *this;
    }
    void set(int axis,double value) { position->axes[axis].end=std::signbit(value); position->axes[axis].offset=std::abs(value);position->axes[axis].request=value;position->pending=true; }
    void queueRestore() {
        if(!native || position->scheduled) return;
        position->scheduled=true; auto object=native.get();
        native->CallAfter([object]() { if(auto node=dynamic_cast<Scroll *>(object->node)) {node->position->scheduled=false;node->restore();} });
    }
public:
    static constexpr double END=CPPUI::ScrollEnd;
    Scroll(Optional<bool> vertical={},Optional<bool> horizontal=false):verticalPolicy(vertical),horizontalPolicy(horizontal) {
        this->vertical=!vertical || *vertical;this->horizontal=!horizontal || *horizontal;
        expandX=this->horizontal ? 4 : 1;expandY=this->vertical ? 4 : 1;
    }
    Scroll & scrollX(double value=0) {return request(0,value);}
    Scroll & scrollY(double value=0) {return request(1,value);}
    int maximum(int axis) const {
        if(!ui) return 0;
        int orientation=axis==0?wxHORIZONTAL:wxVERTICAL;
        return std::max(0,ui->GetScrollRange(orientation)-ui->GetScrollThumb(orientation));
    }
    void updateWidget(Widget * previous) override {
        auto prev=static_cast<Scroll *>(previous);
        position=prev?prev->position:std::make_shared<Position>();
        ui=prev?prev->ui:new wxScrolledWindow(parentWindow());panel=ui;
        for(int axis=0;axis<2;++axis) if(requests[axis]) {
            auto old=position->axes[axis].request;
            if(!old || *old!=*requests[axis] || std::signbit(*old)!=std::signbit(*requests[axis])) set(axis,*requests[axis]);
        }
    }
    void bindEvents() override {ui->Bind(wxEVT_SIZE,&Native::sized,native.get());}
    void updateEvents() override {
        auto object=native.get();native->resize=[object](){if(auto node=dynamic_cast<Scroll *>(object->node))node->queueRestore();};
    }
    void preSync() override {
        if(!ui || position->pending)return;
        auto start=static_cast<wxScrolledWindow *>(ui)->GetViewStart();
        for(int axis=0;axis<2;++axis) {
            int value=axis==0?start.x:start.y;int end=maximum(axis);auto & p=position->axes[axis];
            if(end>20){if(value<=10)p.end=false;else if(value>=end-10)p.end=true;}
            p.offset=p.end?end-value:value;
        }
    }
    void postSync() override {
        SingleChild::postSync();if(!ui)return;
        auto scroll=static_cast<wxScrolledWindow *>(ui);
        scroll->SetScrollRate(horizontal?1:0,vertical?1:0);
        scroll->ShowScrollbars(policy(horizontalPolicy),policy(verticalPolicy));scroll->FitInside();
        restore();queueRestore();
    }
    void restore() {
        if(!ui || destroyed)return;
        int values[2];for(int axis=0;axis<2;++axis){auto & p=position->axes[axis];double value=p.end?maximum(axis)-p.offset:p.offset;values[axis]=static_cast<int>(std::max(0.,std::min(double(maximum(axis)),value)));}
        static_cast<wxScrolledWindow *>(ui)->Scroll(values[0],values[1]);position->pending=false;
    }
};
}
inline Wx::Scroll & Scroll(Optional<bool> vertical={},Optional<bool> horizontal=false){return *new Wx::Scroll(vertical,horizontal);}
}
#endif

#ifndef CPPUI_WX_ADAPTIVE
#define CPPUI_WX_ADAPTIVE
#include "../view.hpp"
#include <wx/wx.h>
#include <wx/weakref.h>

namespace CPPUI { namespace Wx {
    class SizeObserver: public wxEvtHandler, public detail::SizeObserver {
        wxWeakRef<wxWindow> host;
        std::weak_ptr<detail::SizeState> state;
        std::function<wxSize()> measure;
        bool pending=false;
        void resized(wxSizeEvent & event) { event.Skip();refresh(); }
    public:
        SizeObserver(wxWindow * host,std::shared_ptr<detail::SizeState> state,std::function<wxSize()> measure)
            :host(host),state(state),measure(std::move(measure)) {
            host->Bind(wxEVT_SIZE,&SizeObserver::resized,this);
        }
        ~SizeObserver() override {
            if(host) host->Unbind(wxEVT_SIZE,&SizeObserver::resized,this);
            if(wxTheApp) wxTheApp->RemovePendingEventHandler(this);
            DeletePendingEvents();
        }
        void refresh() override {
            if(pending || !host) return;
            pending=true;
            CallAfter([this]() {
                pending=false;
                if(!host) return;
                auto size=measure();auto current=state.lock();
                if(current) current->publish(size.GetWidth(),size.GetHeight());
            });
        }
    };
}}
#endif

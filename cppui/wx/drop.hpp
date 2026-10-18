#ifndef CPPUI_WX_DROP
#define CPPUI_WX_DROP
#include "../drop.hpp"
#include <wx/dnd.h>
#include <wx/dataobj.h>
#include <wx/filename.h>
#include <wx/window.h>
#include <wx/weakref.h>
#include <sstream>
#include <iostream>

namespace CPPUI { namespace Wx {
    class DropTarget:public wxDropTarget {
        wxWeakRef<wxWindow> window;
        public:
        ::CPPUI::detail::DropHandlers handlers;
        wxDataObjectComposite * data;
        wxFileDataObject * files;
        wxTextDataObject * text;
        wxCustomDataObject * uris;
        explicit DropTarget(wxWindow * window):window(window) {
            data=new wxDataObjectComposite(); files=new wxFileDataObject(); text=new wxTextDataObject();
            uris=new wxCustomDataObject(wxDataFormat("text/uri-list"));
            data->Add(files,true); data->Add(text); data->Add(uris); SetDataObject(data);
        }
        static std::string action(wxDragResult result) {
            return result==wxDragMove ? "move" : result==wxDragLink ? "link" : "copy";
        }
        wxDragResult OnEnter(wxCoord x,wxCoord y,wxDragResult result) override {
            auto callbacks=handlers; auto alive=window;
            DropEvent event; event.x=double(x); event.y=double(y); event.action=action(result);
            try { if(callbacks.entered) callbacks.entered(event); }
            catch(...) { std::cerr<<"CPPUI drag-enter handler failed\n"; return wxDragNone; }
            auto current=alive ? dynamic_cast<DropTarget *>(alive->GetDropTarget()) : nullptr;
            return current && current->handlers.dropped && alive->IsEnabled() ? result : wxDragNone;
        }
        wxDragResult OnDragOver(wxCoord,wxCoord,wxDragResult result) override {
            return window && window->IsEnabled() && handlers.dropped ? result : wxDragNone;
        }
        DropEvent eventData(wxCoord x,wxCoord y,wxDragResult result,const wxDataFormat & format) const {
            DropEvent event; event.x=double(x); event.y=double(y); event.action=action(result);
            if(format==files->GetFormat()) {
                for(const auto & path:files->GetFilenames())
                    event.files.emplace_back(wxFileName(path).GetFullName().ToStdString(wxConvUTF8),path.ToStdString(wxConvUTF8));
            } else if(format==text->GetFormat()) event.text=text->GetText().ToStdString(wxConvUTF8);
            else {
                std::string value;
                if(uris->GetSize()) value.assign(static_cast<const char *>(uris->GetData()),uris->GetSize());
                std::istringstream lines(value); std::string line;
                while(std::getline(lines,line)) {
                    if(!line.empty() && line.back()=='\r') line.pop_back();
                    if(!line.empty() && line[0]!='#') event.urls.push_back(line);
                }
            }
            return event;
        }
        // Also usable by host event bridges after native data negotiation.
        wxDragResult dispatchData(DropEvent & event,wxDragResult result) {
            auto callbacks=handlers; const bool enabled=window && window->IsEnabled();
            try { callbacks.dispatch(event,enabled); }
            catch(const std::exception & error) { std::cerr<<"CPPUI drop: "<<error.what()<<'\n'; }
            catch(...) { std::cerr<<"CPPUI drop handler failed\n"; }
            return event.accepted ? result : wxDragNone;
        }
        wxDragResult OnData(wxCoord x,wxCoord y,wxDragResult result) override {
            if((result!=wxDragCopy && result!=wxDragMove && result!=wxDragLink) || !GetData()) return wxDragNone;
            auto event=eventData(x,y,result,data->GetReceivedFormat());
            return dispatchData(event,result);
        }
    };
}}
#endif

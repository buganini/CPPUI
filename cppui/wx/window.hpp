#ifndef CPPUI_WX_WINDOW
#define CPPUI_WX_WINDOW
#include "base.hpp"
namespace CPPUI { namespace Wx {
    class Window: public SingleChild {
        OptionalText caption, iconPath;
        Optional<wxSize> dimensions;
        Optional<bool> initialMaximized, initialFullscreen;
        public:
        Window & title(const std::string & text) { caption = text; return *this; }
        Window & icon(const std::string & path) { iconPath = path; return *this; }
        Window & size(int w, int h) { dimensions = wxSize(w, h); return *this; }
        Window & maximize(bool value = true) { if(ui) static_cast<wxFrame *>(ui)->Maximize(value); else initialMaximized = value; return *this; }
        Window & fullscreen(bool value = true) { if(ui) static_cast<wxFrame *>(ui)->ShowFullScreen(value); else initialFullscreen = value; return *this; }
        void updateWidget(Widget * previous) override {
            auto prev = static_cast<Window *>(previous);
            if(prev) { ui = prev->ui; panel = prev->panel; }
            else {
                ui = new wxFrame(nullptr, wxID_ANY, ""); panel = new wxPanel(ui);
                auto box = new wxBoxSizer(wxVERTICAL); box->Add(panel, 1, wxEXPAND); ui->SetSizer(box);
            }
            auto frame = static_cast<wxFrame *>(ui);
            if(caption) frame->SetTitle(wxString::FromUTF8(*caption));
            if(iconPath && !iconPath->empty()) { wxIcon icon; if(icon.LoadFile(wxString::FromUTF8(*iconPath))) frame->SetIcon(icon); }
            if(dimensions && (!prev || (!prev->dimensions || *prev->dimensions != *dimensions))) frame->SetSize(*dimensions);
            if(!prev) {
                if(initialMaximized) frame->Maximize(*initialMaximized);
                if(initialFullscreen) frame->ShowFullScreen(*initialFullscreen);
            }
        }
    };
}
inline Wx::Window & Window() { return *new Wx::Window(); }
}
#endif

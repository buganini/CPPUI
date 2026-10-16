#ifndef CPPUI_WX_WINDOW
#define CPPUI_WX_WINDOW
#include "base.hpp"
#include "../application_title.hpp"
#include "menu.hpp"
namespace CPPUI { namespace Wx {
    class Window: public SingleChild {
        OptionalText caption, iconPath;
        Optional<wxSize> dimensions;
        Optional<bool> initialMaximized, initialFullscreen;
        public:
        Window & title(const std::string & text) { caption = text; return *this; }
        Window & icon(const std::string & path) { iconPath = path; return *this; }
        Window & size(int w, int h) { dimensions = wxSize(w, h); return *this; }
        Window & maximize(bool value = true) { initialMaximized = value; return *this; }
        Window & fullscreen(bool value = true) { initialFullscreen = value; return *this; }
        void addChild(int index, ::CPPUI::Node * child) override {
            if(auto bar=dynamic_cast<MenuBar *>(child)){static_cast<wxFrame *>(ui)->SetMenuBar(bar->ui);return;}
            SingleChild::addChild(index,child);
        }
        void removeChild(int index, ::CPPUI::Node * child) override {
            if(dynamic_cast<MenuBar *>(child)){if(ui)static_cast<wxFrame *>(ui)->SetMenuBar(nullptr);return;}
            SingleChild::removeChild(index,child);
        }
        void updateWidget(Widget * previous) override {
            auto prev = static_cast<Window *>(previous);
            if(prev) { ui = prev->ui; panel = prev->panel; }
            else {
                ui = new wxFrame(nullptr, wxID_ANY, ""); panel = new wxPanel(ui);
                auto box = new wxBoxSizer(wxVERTICAL); box->Add(panel, 1, wxEXPAND); ui->SetSizer(box);
            }
            auto frame = static_cast<wxFrame *>(ui);
            auto title = windowTitle(this, caption);
            if(title) frame->SetTitle(wxString::FromUTF8(*title));
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

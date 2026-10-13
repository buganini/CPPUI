#ifndef CPPUI_WX_BUTTON
#define CPPUI_WX_BUTTON
#include "base.hpp"
namespace CPPUI { namespace Wx {
    class Button: public Widget {
        bool isEnabled = true;
        public:
        std::string text;
        explicit Button(const std::string & text): text(text) {}
        Button & enabled(bool value) { isEnabled = value; return *this; }
        void updateWidget(Widget * prev) override {
            ui = prev ? prev->ui : new wxButton(parentWindow(), wxID_ANY, wxString::FromUTF8(text));
            ui->SetLabel(wxString::FromUTF8(text));
            ui->Enable(isEnabled);
        }
        void bindEvents() override { ui->Bind(wxEVT_BUTTON, &Native::clicked, native.get()); }
        void updateEvents() override {
            auto callback = onClicked;
            native->command = [callback]() { Node::commitFocused(); if(callback) callback(); };
        }
    };
}
inline Wx::Button & Button(const std::string & text) { return *new Wx::Button(text); }
}
#endif

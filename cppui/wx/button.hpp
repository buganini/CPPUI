#ifndef CPPUI_WX_BUTTON
#define CPPUI_WX_BUTTON
#include "base.hpp"
#include "painted_controls.hpp"
namespace CPPUI { namespace Wx {
    class Button: public Widget {
        bool isDisabled = false;
        public:
        std::string text;
        explicit Button(const std::string & text): text(text) {}
        Button & disabled(bool value = true) { isDisabled = value; return *this; }
        bool customColors() const {
#ifdef __WXOSX__
            return bool(style_color) || bool(style_bgcolor);
#else
            return false;
#endif
        }
        std::size_t typecode() const override { return Widget::typecode() ^ (customColors() ? 0x69acU : 0U); }
        void updateWidget(Widget * prev) override {
            if(prev) ui=prev->ui;
            else if(customColors()) ui=new Painting::Button(parentWindow(),wxString::FromUTF8(text));
            else ui=new wxButton(parentWindow(),wxID_ANY,wxString::FromUTF8(text));
            ui->SetLabel(wxString::FromUTF8(text));
            ui->Enable(!isDisabled);
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

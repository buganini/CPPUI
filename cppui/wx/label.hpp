#ifndef CPPUI_WX_LABEL
#define CPPUI_WX_LABEL
#include "base.hpp"
#include "painted_controls.hpp"
namespace CPPUI { namespace Wx {
    class Label: public Widget {
        protected:
        wxWindow * createStaticText() const {
            auto parent=parentWindow();
#ifdef __WXMSW__
            if(dynamic_cast<Painting::StackPanel *>(parent))
                return new Painting::StaticText(parent,wxString::FromUTF8(text),true);
#endif
#ifdef __WXOSX__
            if(style_bgcolor) return new Painting::StaticText(parent,wxString::FromUTF8(text));
#endif
            return new wxStaticText(parent,wxID_ANY,wxString::FromUTF8(text));
        }
        void configurePainting() {
            if(auto painted=dynamic_cast<Painting::StaticText *>(ui)) painted->customBackground=bool(style_bgcolor);
        }
        public:
        std::string text;
        bool selectable;
        explicit Label(const std::string & text, bool selectable = false): text(text), selectable(selectable) {}
        std::size_t typecode() const override {
            auto code=Widget::typecode() ^ (selectable ? 0x927cU : 0U);
#ifdef __WXOSX__
            if(!selectable && style_bgcolor) code ^= 0x69acU;
#endif
            return code;
        }
        void updateWidget(Widget * prev) override {
            if(prev) ui = prev->ui;
            else if(selectable) ui = new wxTextCtrl(parentWindow(), wxID_ANY, wxString::FromUTF8(text), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
            else ui = createStaticText();
            if(selectable) static_cast<wxTextCtrl *>(ui)->ChangeValue(wxString::FromUTF8(text));
            else ui->SetLabel(wxString::FromUTF8(text));
            configurePainting();
        }
        void bindEvents() override { ui->Bind(wxEVT_LEFT_DOWN, &Native::mouse, native.get()); }
    };
}
inline Wx::Label & Label(const std::string & text, bool selectable = false) { return *new Wx::Label(text, selectable); }
}
#endif

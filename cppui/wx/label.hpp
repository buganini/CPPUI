#ifndef CPPUI_WX_LABEL
#define CPPUI_WX_LABEL
#include "base.hpp"
namespace CPPUI { namespace Wx {
    class Label: public Widget {
        public:
        std::string text;
        bool selectable;
        explicit Label(const std::string & text, bool selectable = false): text(text), selectable(selectable) {}
        std::size_t typecode() const override { return Widget::typecode() ^ (selectable ? 0x927cU : 0U); }
        void updateWidget(Widget * prev) override {
            if(prev) ui = prev->ui;
            else if(selectable) ui = new wxTextCtrl(parentWindow(), wxID_ANY, wxString::FromUTF8(text), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxBORDER_NONE);
            else ui = new wxStaticText(parentWindow(), wxID_ANY, wxString::FromUTF8(text));
            if(selectable) static_cast<wxTextCtrl *>(ui)->ChangeValue(wxString::FromUTF8(text));
            else ui->SetLabel(wxString::FromUTF8(text));
        }
        void bindEvents() override { ui->Bind(wxEVT_LEFT_DOWN, &Native::mouse, native.get()); }
    };
}
inline Wx::Label & Label(const std::string & text, bool selectable = false) { return *new Wx::Label(text, selectable); }
}
#endif

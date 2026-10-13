#ifndef CPPUI_WX_TEXT
#define CPPUI_WX_TEXT
#include "label.hpp"
#include "../diagnostics.hpp"
#include <wx/html/htmlwin.h>
namespace CPPUI { namespace Wx {
class Text: public Label {
public:
    Text(const std::string & text, bool selectable): Label(text, selectable) {}
    void updateWidget(Widget * prev) override {
        if(prev) ui = prev->ui;
        else if(selectable) ui = new wxTextCtrl(parentWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxBORDER_NONE);
        else ui = new wxStaticText(parentWindow(), wxID_ANY, "");
        if(selectable) static_cast<wxTextCtrl *>(ui)->ChangeValue(wxString::FromUTF8(text));
        else ui->SetLabel(wxString::FromUTF8(text));
    }
};
class Html: public Widget {
    std::string text;
public:
    Html(const std::string & text, bool): text(text) {}
    void updateWidget(Widget * prev) override {
        ui = prev ? prev->ui : new wxHtmlWindow(parentWindow());
        static_cast<wxHtmlWindow *>(ui)->SetPage(wxString::FromUTF8(text));
    }
};
}
inline Wx::Text & Text(const std::string & text, bool selectable = false) { return *new Wx::Text(text, selectable); }
inline Wx::Html & Html(const std::string & text, bool selectable = false) { return *new Wx::Html(text, selectable); }
// Match PUI/wx's plain Text fallback while identifying the missing renderer.
CPPUI_UNSUPPORTED("CPPUI/wx MarkDown has no Markdown renderer; displaying raw text")
inline Wx::Text & MarkDown(const std::string & text, bool selectable = false) { return *new Wx::Text(text, selectable); }
}
#endif

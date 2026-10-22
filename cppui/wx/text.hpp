#ifndef CPPUI_WX_TEXT
#define CPPUI_WX_TEXT
#include "label.hpp"
#include "../diagnostics.hpp"
#include <wx/html/htmlwin.h>
namespace CPPUI { namespace Wx {
namespace detail {
class HtmlWindow: public wxHtmlWindow {
public:
    explicit HtmlWindow(wxWindow * parent): wxHtmlWindow(parent) {}
protected:
    wxSize DoGetBestClientSize() const override {
        auto content = GetInternalRepresentation();
        return content ? wxSize(content->GetWidth(), content->GetHeight())
                       : wxHtmlWindow::DoGetBestClientSize();
    }
};
}
class Text: public Label {
public:
    Text(const std::string & text, bool selectable): Label(text, selectable) {}
    void updateWidget(Widget * prev) override {
        if(prev) ui = prev->ui;
        else if(selectable) ui = new wxTextCtrl(parentWindow(), wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE | wxBORDER_NONE);
        else ui = createStaticText();
        if(selectable) static_cast<wxTextCtrl *>(ui)->ChangeValue(wxString::FromUTF8(text));
        else static_cast<wxControl *>(ui)->SetLabelText(wxString::FromUTF8(text));
        configurePainting();
    }
};
class Html: public Widget {
    std::string text;
public:
    Html(const std::string & text, bool): text(text) { expandX = expandY = 3; }
    void updateWidget(Widget * prev) override {
        ui = prev ? prev->ui : new detail::HtmlWindow(parentWindow());
    }
    void update(::CPPUI::Node * prev) override {
        Widget::update(prev);
        auto source=wxString::FromUTF8(text);
        if(style_color) source=wxString::Format("<font color=\"#%06x\">",unsigned(*style_color)&0xffffff)+source+"</font>";
        static_cast<wxHtmlWindow *>(ui)->SetPage(source);
        // Parsing a document resets HtmlWindow's background.
        if(style_bgcolor) ui->SetBackgroundColour(colour(*style_bgcolor));
        ui->InvalidateBestSize();
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

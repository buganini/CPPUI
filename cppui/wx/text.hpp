#ifndef CPPUI_WX_TEXT
#define CPPUI_WX_TEXT
#include "label.hpp"
#include "../diagnostics.hpp"
#include "url.hpp"
#include <regex>
#if CPPUI_WX_WEBVIEW
#include <wx/webview.h>
#endif
#include <wx/html/htmlwin.h>
namespace CPPUI { namespace Wx {
namespace detail {
class HtmlWindow: public wxHtmlWindow {
public:
    bool allowURLs=true;
    explicit HtmlWindow(wxWindow * parent): wxHtmlWindow(parent) {}
    void OnLinkClicked(const wxHtmlLinkInfo & link) override {
        if(allowURLs) try {CPPUI::OpenURL(link.GetHref().ToStdString(wxConvUTF8));}catch(...) {reportException("Html link");}
    }
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
        else if(auto label=dynamic_cast<wxStaticText *>(ui)) label->SetLabelText(wxString::FromUTF8(text));
        else ui->SetLabel(wxString::FromUTF8(text));
        configurePainting();
    }
};
class Html: public Widget {
    std::string text,source;
    bool selectable,allowURLs=true,webview=false;
#if CPPUI_WX_WEBVIEW
    std::shared_ptr<bool> navigation;
#endif
public:
    Html(const std::string & text,bool selectable):text(text),selectable(selectable) {expandX=expandY=4;}
    Html & openURL(bool enabled=true) {allowURLs=enabled;return *this;}
    void updateWidget(Widget * prev) override {
        if(prev) {auto old=static_cast<Html *>(prev);ui=prev->ui;webview=old->webview;source=std::move(old->source);
#if CPPUI_WX_WEBVIEW
            navigation=old->navigation;
#endif
            return;
        }
#if CPPUI_WX_WEBVIEW
#ifdef __WXMSW__
        const auto backend=wxWebViewBackendEdge;
#else
        const auto backend=wxWebViewBackendWebKit;
#endif
        if(wxWebView::IsBackendAvailable(backend)) ui=wxWebView::New(parentWindow(),wxID_ANY,"about:blank",wxDefaultPosition,wxDefaultSize,backend);
        if(ui) {
            webview=true;navigation=std::make_shared<bool>(true);auto enabled=navigation;
            auto navigate=[enabled](wxWebViewEvent & event) {
                if(event.GetURL()=="about:blank") {event.Skip();return;}
                event.Veto();if(*enabled) try {CPPUI::OpenURL(event.GetURL().ToStdString(wxConvUTF8));}catch(...) {reportException("Html link");}
            };
            ui->Bind(wxEVT_WEBVIEW_NAVIGATING,navigate);ui->Bind(wxEVT_WEBVIEW_NEWWINDOW,navigate);return;
        }
#endif
        std::cerr<<"CPPUI wx Html: WebView unavailable; using HtmlWindow (limited HTML/CSS, no JavaScript or selection control)\n";
        ui=new detail::HtmlWindow(parentWindow());
    }
    void update(::CPPUI::Node * prev) override {
        Widget::update(prev);
#if CPPUI_WX_WEBVIEW
        if(webview) {
            *navigation=allowURLs;
            std::string family;for(char c:ui->GetFont().GetFaceName().ToStdString(wxConvUTF8)) {
                if(c=='<' || c=='>' || c=='\\' || c=='"') {char escaped[8];std::snprintf(escaped,sizeof(escaped),"\\%x ",unsigned(static_cast<unsigned char>(c)));family+=escaped;}else family+=c;
            }
            auto css=std::string("<style>body{margin:0;color:")+ui->GetForegroundColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString()+";background:"+ui->GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX).ToStdString()+";font-family:\""+family+"\";font-size:"+std::to_string(ui->GetFont().GetFractionalPointSize())+"pt;font-weight:"+std::to_string(ui->GetFont().GetNumericWeight())+";}"+(selectable?"":"html{-webkit-user-select:none;user-select:none;}")+"</style>";
            std::string next=text;std::smatch match;
            if(std::regex_search(next,match,std::regex("<head(?:\\s[^>]*)?>",std::regex::icase))) next.insert(match.position()+match.length(),css);
            else if(std::regex_search(next,match,std::regex("^\\s*<!doctype[^>]*>",std::regex::icase))) next.insert(match.length(),css);
            else next="<!doctype html>"+css+next;
            if(next!=source) {static_cast<wxWebView *>(ui)->SetPage(wxString::FromUTF8(next),"about:blank");source=std::move(next);}return;
        }
#endif
        auto html=static_cast<detail::HtmlWindow *>(ui);html->allowURLs=allowURLs;
        auto next=wxString::FromUTF8(text);
        if(style_color) next=wxString::Format("<font color=\"#%06x\">",unsigned(*style_color)&0xffffff)+next+"</font>";
        auto encoded=next.ToStdString(wxConvUTF8);
        if(encoded!=source) {html->SetPage(next);source=std::move(encoded);}
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

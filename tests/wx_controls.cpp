#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/button.hpp"
#include "cppui/wx/divider.hpp"
#include "cppui/wx/progressbar.hpp"
#include "cppui/wx/checkbox.hpp"
#include "cppui/wx/radiobutton.hpp"
#include "cppui/wx/textfield.hpp"
#include "cppui/wx/combobox.hpp"
#include "cppui/wx/text.hpp"
#include <stdexcept>
static void require(bool c, const char * m) { if(!c) throw std::runtime_error(m); }
static void pump() { for(int i=0;i<4;++i) { wxTheApp->Yield(true); wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); } }
static CPPUI::Node * find(CPPUI::Node & root, const std::string & id) {
    if(root._id == id) return &root;
    for(auto child: *root.children) if(auto result = find(*child,id)) return result;
    return nullptr;
}
template<class T> static T * ui(CPPUI::Node & root, const std::string & id) { return static_cast<T *>(find(root,id)->outer()); }
static void command(wxWindow * window, wxEventType type) { wxCommandEvent e(type, window->GetId()); e.SetEventObject(window); window->ProcessWindowEvent(e); }
class Root: public CPPUI::Application {
public:
    using Application::Application;
    CPPUI::State<bool> checked{false};
    CPPUI::State<int> radio{1}, index{-1};
    CPPUI::State<std::string> text{std::string("initial")}, draft{std::string()}, selected{std::string("raw")};
    std::string markdown="# Heading\n**bold** <i>literal</i>";
    bool markdownSelectable=false;
    int inputs=0, changes=0, comboChanges=0;
    bool show=true, reverse=false, editable=true;
    double progress=.125, maximum=.25;
    int column=0;
    void content() override {
        for(auto w: CPPUI::Window().size(500,500)) for(auto v: CPPUI::VBox()) {
            if(show) CPPUI::TextField(text,draft).input([this](){++inputs;}).change([this](){++changes;}).id("field");
            CPPUI::Checkbox("check",checked).id("check");
            CPPUI::RadioButton("one",1,radio).id("r1"); CPPUI::RadioButton("two",2,radio).id("r2");
            for(auto c: CPPUI::ComboBox(editable,index,selected).change([this](){++comboChanges;}).id("combo")) {
                CPPUI::ComboBoxItem("One","one"); CPPUI::ComboBoxItem("Two","two");
            }
            for(auto c: CPPUI::ComboBox().id("unbound")) {
                if(reverse) { CPPUI::ComboBoxItem("B"); CPPUI::ComboBoxItem("A"); }
                else { CPPUI::ComboBoxItem("A"); CPPUI::ComboBoxItem("B"); }
            }
            CPPUI::ProgressBar(progress,maximum).id("progress"); CPPUI::Divider();
            CPPUI::Text("a\nb",true).id("text"); CPPUI::Html("<b>bold</b>").id("html");
            CPPUI::MarkDown(markdown,markdownSelectable).id("markdown");
            for(auto g: CPPUI::Grid().id("grid")) { auto & label=CPPUI::Label("cell"); label.grid_row=0;label.grid_column=column;label.grid_columnspan=2;label.id("cell"); }
        }
    }
};
class HtmlSizingRoot: public CPPUI::Application {
public:
    using Application::Application;
    int lines=1, width=320;
    bool spacer=true;
    void content() override {
        std::string text;
        for(int i=0;i<lines;++i) {
            if(i) text+="<br>";
            text+="<i>HTML</i> <span style=\"color:red\">support</span>";
        }
        for(auto w: CPPUI::Window().size(width,240)) for(auto v: CPPUI::VBox()) {
            CPPUI::Html(text).id("html");
            if(spacer) CPPUI::Spacer();
        }
    }
};
static void htmlSizing(int argc,char **argv) {
    HtmlSizingRoot root(argc,argv);root.sync();pump();
    auto html=ui<wxHtmlWindow>(root,"html");
    require(html->ToText().Strip(wxString::both)=="HTML support", "HTML sample did not render its text");
    require(html->GetInternalRepresentation()->GetHeight()>0
        && html->GetClientSize().GetHeight()>=html->GetInternalRepresentation()->GetHeight(),
        "HTML layout clipped its initial content");
    const auto initialBest=html->GetBestSize();
    const auto initialClient=html->GetClientSize();
    root.lines=3;root.sync();pump();
    require(ui<wxHtmlWindow>(root,"html")==html, "HTML content update replaced the native window");
    require(html->GetBestSize().GetHeight()>initialBest.GetHeight()
        && html->GetClientSize().GetHeight()>=html->GetInternalRepresentation()->GetHeight(),
        "HTML best size or layout did not grow with content");
    root.lines=1;root.width=480;root.sync();pump();
    require(html->GetBestSize().GetHeight()==initialBest.GetHeight(), "HTML best size stayed stale after content shrank");
    require(html->GetClientSize().GetWidth()>initialClient.GetWidth(), "HTML did not expand horizontally with its layout");
    const int compactHeight=html->GetClientSize().GetHeight();
    root.spacer=false;root.sync();pump();
    require(html->GetClientSize().GetHeight()>compactHeight, "HTML did not use available vertical layout space");
}
int main(int argc,char **argv) {
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString &c,const wxString &m){std::cerr<<c<<": "<<m<<'\n';std::abort();});
    try {
        Root root(argc,argv);root.sync();pump();
        auto multiline=ui<wxTextCtrl>(root,"text");
        require(multiline->IsMultiLine() && !multiline->IsEditable() && multiline->GetValue()=="a\nb"
            && multiline->GetNumberOfLines()==2, "selectable Text did not retain native multiline/read-only behavior");
        auto markdown=ui<wxStaticText>(root,"markdown");
        require(markdown->GetLabel()==wxString::FromUTF8(root.markdown), "Markdown fallback did not display literal source");
        root.markdown="# Updated\n* literal item";root.sync();
        require(ui<wxStaticText>(root,"markdown")==markdown && markdown->GetLabel()==wxString::FromUTF8(root.markdown),
            "Markdown update lost native identity or raw source");
        wxWeakRef<wxWindow> oldMarkdown(markdown);root.markdownSelectable=true;root.sync();pump();
        auto selectableMarkdown=ui<wxTextCtrl>(root,"markdown");
        require(!oldMarkdown && selectableMarkdown->IsMultiLine() && !selectableMarkdown->IsEditable()
            && selectableMarkdown->GetValue()==wxString::FromUTF8(root.markdown), "selectable Markdown fallback did not remount as read-only multiline text");
        require(static_cast<std::string>(root.draft)=="initial", "draft not initialized");
        auto field=ui<wxTextCtrl>(root,"field");field->SetValue("typed");pump();
        require(root.inputs==1 && static_cast<std::string>(root.text)=="initial" && static_cast<std::string>(root.draft)=="typed", "typing committed model or lost draft");
        root.text=std::string("external");pump();require(field->GetValue()=="typed", "redraw overwrote active draft");
        command(field,wxEVT_TEXT_ENTER);pump();command(field,wxEVT_TEXT_ENTER);
        require(root.changes==1 && static_cast<std::string>(root.text)=="typed", "commit duplicated or stale");
        auto check=ui<wxCheckBox>(root,"check");check->SetValue(true);command(check,wxEVT_CHECKBOX);pump();require(root.checked, "checkbox binding failed");
        command(ui<wxRadioButton>(root,"r2"),wxEVT_RADIOBUTTON);pump();require(root.radio==2 && !ui<wxRadioButton>(root,"r1")->GetValue(), "radio binding failed");
        auto combo=ui<wxComboBox>(root,"combo");require(combo->GetSelection()==-1 && combo->GetValue()=="raw", "invalid selection discarded free text");
        combo->SetSelection(1);command(combo,wxEVT_COMBOBOX);pump();require(root.index==1 && static_cast<std::string>(root.selected)=="two", "combo wrote label instead of value");
        combo->SetValue("free");pump();require(root.index==-1 && static_cast<std::string>(root.selected)=="free" && combo->GetValue()=="free", "editable combo lost typed value");
        auto unbound=ui<wxComboBox>(root,"unbound");unbound->SetSelection(1);command(unbound,wxEVT_COMBOBOX);root.reverse=true;root.sync();require(unbound->GetSelection()==0 && unbound->GetValue()=="B", "unbound keyed selection was lost");
        auto bar=ui<wxGauge>(root,"progress");require(bar->GetRange()==25 && bar->GetValue()==12,"fractional progress range failed");
        root.progress=-3;root.maximum=0;root.column=3;root.sync();require(bar->GetRange()==1&&bar->GetValue()==0,"gauge clamp failed");
        auto grid=static_cast<CPPUI::Wx::Grid *>(find(root,"grid"));auto item=static_cast<wxGBSizerItem *>(grid->mounts.at(0).item);require(item->GetPos().GetCol()==3&&item->GetSpan().GetColspan()==2,"grid placement failed");
        wxWeakRef<wxWindow> old(combo);root.editable=false;root.sync();pump();require(!old,"combo creation style change did not remount");
        root.show=false;root.sync();pump();root.redraw();
        htmlSizing(argc,argv);
    } catch(const std::exception & e) {std::cerr<<e.what()<<'\n';return 1;}
}

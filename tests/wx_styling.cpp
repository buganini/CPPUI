#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/button.hpp"
#include "cppui/wx/text.hpp"
#include <wx/dcmemory.h>
#include <set>

static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
static void pump() {for(int i=0;i<4;++i) {wxTheApp->Yield(true);wxTheApp->ProcessPendingEvents();wxTheApp->ProcessIdle();}}
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto found=find(*child,id)) return found;
    return nullptr;
}
static wxWindow * native(CPPUI::Node & root,const char * id) {return static_cast<wxWindow *>(find(root,id)->outer());}
static std::set<int> colors(const wxBitmap & bitmap) {
    const auto image=bitmap.ConvertToImage();const auto pixels=image.GetData();std::set<int> result;
    for(int i=0;i<image.GetWidth()*image.GetHeight();++i) result.insert((pixels[3*i]<<16)|(pixels[3*i+1]<<8)|pixels[3*i+2]);
    return result;
}
template<class Paint> static std::set<int> render(wxSize size,Paint paint) {
    wxBitmap bitmap(size);wxMemoryDC dc(bitmap);dc.SetBackground(*wxWHITE_BRUSH);dc.Clear();paint(dc);
    dc.SelectObject(wxNullBitmap);return colors(bitmap);
}
class Root:public CPPUI::Application {
public:
    using Application::Application;
    CPPUI::Optional<int> weight{700};
    int background=0x244b5a,foreground=0x1c2a30,clicks=0;
    bool styled=true;
    void content() override {
        for(auto window:CPPUI::Window().size(480,360).id("window")) for(auto column:CPPUI::VBox()) {
            auto & button=CPPUI::Button("Styled").id("button");button.click([this]() {++clicks;});
            CPPUI::Node * nodes[]={&CPPUI::Label("Styled").id("label"),&CPPUI::Text("A & B\nStyled").id("text"),
                &button};
            for(auto node:nodes) {
                if(styled) node->style(0xfff7e6,background);
                if(weight) node->style(CPPUI::style::fontWeight(*weight));
            }
            auto & html=CPPUI::Html("<font size=\"7\">MMMM <font color=\"#ff0000\">MMMM</font></font>").id("html");
            if(styled) html.style(foreground,0xfff7e6);
        }
    }
};
static void styles(Root & root) {
    root.sync();pump();
    auto label=native(root,"label"),text=native(root,"text"),button=native(root,"button");
    require(static_cast<wxControl *>(text)->GetLabelText()=="A & B\nStyled","plain Text lost literal ampersands");
#ifdef __WXOSX__
    require(dynamic_cast<CPPUI::Wx::Painting::StaticText *>(label) && dynamic_cast<CPPUI::Wx::Painting::StaticText *>(text)
        && dynamic_cast<CPPUI::Wx::Painting::Button *>(button),"Cocoa custom colours used native controls");
#endif
    for(int weight:{100,300,400,550,600,700,900,950,1000}) {
        root.weight=weight;root.background=0xa84a3a;root.sync();pump();
        require(native(root,"label")==label && native(root,"text")==text && native(root,"button")==button,"style update replaced controls");
        for(auto window:{label,text,button}) require(window->GetFont().GetNumericWeight()==weight
            && window->GetBackgroundColour()==wxColour(0xa8,0x4a,0x3a),"numeric font weight or color was lost");
    }
    wxCommandEvent click(wxEVT_BUTTON,button->GetId());click.SetEventObject(button);button->ProcessWindowEvent(click);
    require(root.clicks==1,"styled button lost its click handler");
    auto html=static_cast<wxHtmlWindow *>(native(root,"html"));
    for(int foreground:{0x1c2a30,0x0000ff}) {
        root.foreground=foreground;root.sync();pump();
        require(native(root,"html")==html && html->GetBackgroundColour()==wxColour(0xff,0xf7,0xe6),"HTML style lost identity/background");
        const auto pixels=render(wxSize(400,100),[&](wxDC & dc) {
            auto cell=html->GetInternalRepresentation();cell->Layout(400);wxHtmlRenderingInfo info;
            cell->Draw(dc,0,0,0,100,info);
        });
        require(pixels.count(foreground) && pixels.count(0xff0000),"HTML default or inline text color missing from pixels");
    }
    root.styled=false;root.weight.reset();root.sync();pump();
    require(dynamic_cast<wxStaticText *>(native(root,"label")) && dynamic_cast<wxStaticText *>(native(root,"text"))
        && dynamic_cast<wxButton *>(native(root,"button")),"omitted colors did not restore native controls");
    require(native(root,"label")->GetFont().GetNumericWeight()==wxFONTWEIGHT_NORMAL,"omitted font weight did not restore the native font");
    const auto pixels=render(wxSize(400,100),[&](wxDC & dc) {
        auto cell=html->GetInternalRepresentation();cell->Layout(400);wxHtmlRenderingInfo info;cell->Draw(dc,0,0,0,100,info);
    });
    require(!pixels.count(0x0000ff) && pixels.count(0xff0000),"omitted HTML style retained a wrapper or lost inline color");
}
static void paintedControls(wxWindow * parent) {
    using CPPUI::Wx::Painting::StaticText;
    auto panel=new CPPUI::Wx::Painting::StackPanel(parent);
    auto sizer=new CPPUI::Wx::StackSizer();panel->SetSizer(sizer);panel->SetSize(240,100);
    auto bottom=new StaticText(panel,"MMMM",true),top=new StaticText(panel,"MMMM",true);
    bottom->SetOwnForegroundColour(wxColour(0x24,0x4b,0x5a));top->SetOwnForegroundColour(wxColour(0xa8,0x4a,0x3a));
    auto font=bottom->GetFont();font.SetPointSize(32);bottom->SetFont(font);top->SetFont(font);
    auto nested=new wxBoxSizer(wxVERTICAL);nested->Add(bottom,1,wxEXPAND);sizer->Add(nested,1,wxEXPAND);sizer->Add(top,1,wxEXPAND);panel->Layout();
    auto check=[&](int expected,int covered) {
        for(auto window:{bottom,top}) if(window->IsShown()) {
            const auto pixels=render(window->GetSize(),[&](wxDC & dc) {window->paintTo(dc);});
            require(pixels.count(expected) && !pixels.count(covered),"Stack composition depends on which sibling paints last");
        }
    };
    check(0xa84a3a,0x244b5a);
    sizer->Detach(top);sizer->Insert(0,top,1,wxEXPAND);panel->Layout();check(0x244b5a,0xa84a3a);
    sizer->Detach(top);sizer->Add(top,1,wxEXPAND);sizer->Show(top,false);panel->Layout();check(0x244b5a,0xa84a3a);
    sizer->Show(top,true);panel->SetSize(320,140);panel->Layout();check(0xa84a3a,0x244b5a);
    top->customBackground=true;top->SetBackgroundColour(wxColour(0xff,0xf7,0xe6));
    const auto stackPixels=render(top->GetSize(),[&](wxDC & dc) {top->paintTo(dc);});
    require(stackPixels.count(0xfff7e6) && !stackPixels.count(0x244b5a),"opaque text layer failed to cover lower text");

    auto label=new StaticText(panel,"A & B\nMMMM");label->SetSize(120,80);label->SetOwnForegroundColour(*wxWHITE);
    auto button=new CPPUI::Wx::Painting::Button(panel,"Styled");button->SetSize(120,60);
    for(auto background:{wxColour(0x24,0x4b,0x5a),wxColour(0xa8,0x4a,0x3a)}) {
        label->SetBackgroundColour(background);button->SetBackgroundColour(background);
        const int rgb=(background.Red()<<16)|(background.Green()<<8)|background.Blue();
        require(render(label->GetSize(),[&](wxDC & dc) {label->paintTo(dc);}).count(rgb),"compact label background was not painted");
        require(render(button->GetSize(),[&](wxDC & dc) {button->paintTo(dc);}).count(rgb),"custom button background was not painted");
    }
    int clicks=0;button->Bind(wxEVT_BUTTON,[&](wxCommandEvent &) {++clicks;});
    auto key=[&](wxEventType type,int code) {wxKeyEvent event(type);event.m_keyCode=code;button->ProcessWindowEvent(event);};
    key(wxEVT_KEY_DOWN,WXK_SPACE);key(wxEVT_KEY_DOWN,WXK_SPACE);key(wxEVT_KEY_UP,WXK_SPACE);
    require(clicks==1,"Space autorepeat duplicated button activation");
    key(wxEVT_KEY_DOWN,WXK_RETURN);require(clicks==2,"Enter did not activate the custom button");
    button->Disable();key(wxEVT_KEY_DOWN,WXK_SPACE);key(wxEVT_KEY_UP,WXK_SPACE);key(wxEVT_KEY_DOWN,WXK_RETURN);
    require(clicks==2,"disabled button activated");button->Enable();
    auto mouse=[&](wxEventType type,wxPoint point) {wxMouseEvent event(type);event.SetPosition(point);button->ProcessWindowEvent(event);};
    mouse(wxEVT_LEFT_DOWN,wxPoint(10,10));mouse(wxEVT_LEFT_UP,wxPoint(-1,-1));
    require(clicks==2 && !button->HasCapture(),"release outside the button activated it or retained capture");
    mouse(wxEVT_LEFT_DOWN,wxPoint(10,10));mouse(wxEVT_LEFT_UP,wxPoint(10,10));
    require(clicks==3 && !button->HasCapture(),"mouse click failed or retained capture");
    key(wxEVT_KEY_DOWN,WXK_SPACE);wxFocusEvent blur(wxEVT_KILL_FOCUS);button->ProcessWindowEvent(blur);key(wxEVT_KEY_UP,WXK_SPACE);
    require(clicks==3,"focus loss did not cancel the pending keyboard activation");
    panel->Destroy();pump();
}
int main(int argc,char ** argv) {
    wxSetAssertHandler([](const wxString &,int,const wxString &,const wxString & condition,const wxString & message) {
        std::cerr<<condition<<": "<<message<<'\n';std::abort();
    });
    try {Root root(argc,argv);styles(root);paintedControls(native(root,"window"));}
    catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

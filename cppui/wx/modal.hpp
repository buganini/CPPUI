#ifndef CPPUI_WX_MODAL
#define CPPUI_WX_MODAL
#include "base.hpp"
#include "../binding.hpp"
#include <wx/dialog.h>
namespace CPPUI { namespace Wx {
    class ModalWindows: public wxEvtHandler {
        std::vector<wxWeakRef<wxWindow>> stack,disabled;
        wxApp * app=nullptr;
        void idle(wxIdleEvent & e) {refresh();e.Skip();}
    public:
        static ModalWindows & instance() {static ModalWindows value;return value;}
        void refresh() {
            stack.erase(std::remove_if(stack.begin(),stack.end(),[](const wxWeakRef<wxWindow> & w){return !w || !w->IsShown();}),stack.end());
            wxWindow * top=stack.empty() ? nullptr : stack.back().get();
            for(auto i=disabled.begin();i!=disabled.end();) {
                if(!*i || !top || i->get()==top || !(*i)->IsShown()) {if(*i) (*i)->Enable();i=disabled.erase(i);} else ++i;
            }
            if(top) for(auto w:wxTopLevelWindows) if(w!=top && w->IsShown() && w->IsEnabled()) {w->Disable();disabled.push_back(w);}
        }
        void open(wxWindow * w) {
            if(!app) {app=wxTheApp;app->Bind(wxEVT_IDLE,&ModalWindows::idle,this);}
            auto at=stack.end();
            for(auto i=stack.begin();i!=stack.end();++i) if(*i && w->IsDescendant(i->get())) {at=i;break;}
            stack.insert(at,w);refresh();if(!stack.empty()) {stack.back()->Raise();stack.back()->SetFocus();}
        }
        void close(wxWindow * w) {
            stack.erase(std::remove_if(stack.begin(),stack.end(),[w](const wxWeakRef<wxWindow> & x){return !x || x.get()==w;}),stack.end());
            refresh();if(stack.empty() && app) {app->Unbind(wxEVT_IDLE,&ModalWindows::idle,this);app=nullptr;}
        }
    };
    class ModalWindow: public wxDialog {
        wxWeakRef<wxWindow> focus;
    public:
        bool opened=false,sized=false;
        Optional<wxSize> declaredSize;
        std::function<void()> dismiss,notifyOpen,notifyClose;
        explicit ModalWindow(wxWindow * owner):wxDialog(owner,wxID_ANY,"",wxDefaultPosition,wxDefaultSize,wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) {
            Bind(wxEVT_CLOSE_WINDOW,[this](wxCloseEvent & e){auto cb=dismiss;if(e.CanVeto()) e.Veto();else e.Skip();if(cb) cb();});
            Bind(wxEVT_CHAR_HOOK,[this](wxKeyEvent & e){if(e.GetKeyCode()==WXK_ESCAPE) {auto cb=dismiss;if(cb) cb();}else e.Skip();});
            Bind(wxEVT_DESTROY,[this](wxWindowDestroyEvent & e){if(e.GetEventObject()==this) ModalWindows::instance().close(this);e.Skip();});
        }
        void setOpen(bool value) {
            if(opened==value) return;opened=value;
            if(value) {focus=wxWindow::FindFocus();Show();ModalWindows::instance().open(this);}
            else {Hide();ModalWindows::instance().close(this);if(focus && focus->IsShownOnScreen() && focus->IsEnabled()) focus->SetFocus();}
            auto cb=value ? notifyOpen : notifyClose;if(cb) cb();
        }
        ~ModalWindow() override {ModalWindows::instance().close(this);}
    };
    class Modal: public SingleChild {
        bool shown;
        std::function<void()> writeOff,onOpen,onClose;
        std::string caption;
        bool requested() const {
            if(!shown || !is_visible) return false;
            for(auto p=parent;p && p!=this;p=p->parent) {
                if(!p->is_visible) return false;
                if(auto modal=dynamic_cast<const Modal *>(p)) if(!modal->shown) return false;
                if(p==p->parent)break;
            }return true;
        }
        Optional<wxSize> dimensions;
        Optional<bool> maximized,fullscreenValue;
    public:
        template<class T> explicit Modal(Binding<T> state,T off=T{}):shown(bool(state.get())),writeOff([state,off](){state.set(off);}) {is_outoforder=true;}
        template<class T> explicit Modal(State<T> & state,T off=T{}):Modal(Binding<T>(state),off) {}
        bool supportsVisibility() const override {return false;}
        Modal & title(const std::string & text) {caption=text;return *this;}
        using ::CPPUI::Node::size;
        Modal & size(int w,int h) {dimensions=wxSize(w,h);return *this;}
        Modal & maximize(bool value=true) {maximized=value;return *this;}
        Modal & fullscreen(bool value=true) {fullscreenValue=value;return *this;}
        Modal & open(std::function<void()> cb) {onOpen=std::move(cb);return *this;}
        Modal & close(std::function<void()> cb) {onClose=std::move(cb);return *this;}
        void updateWidget(Widget * prev) override {
            ui=prev ? prev->ui : new ModalWindow(wxGetTopLevelParent(parentWindow()));panel=ui;
            auto w=static_cast<ModalWindow *>(ui);w->SetTitle(wxString::FromUTF8(caption));
            auto root=dynamic_cast<View *>(cppui_root);auto opened=onOpen,closed=onClose;
            w->notifyOpen=[root,opened](){if(root)root->queueCallback(opened);};
            w->notifyClose=[root,closed](){if(root)root->queueCallback(closed);};auto write=writeOff;
            w->dismiss=[w,write](){w->setOpen(false);try {write();}catch(...) {reportException("Modal dismiss");}};
        }
        void postSync() override {
            SingleChild::postSync();if(!ui) return;auto w=static_cast<ModalWindow *>(ui);
            if(dimensions && (!w->declaredSize || *dimensions!=*w->declaredSize)) w->SetSize(*dimensions);
            w->declaredSize=dimensions;
            if(!w->sized) {if(!dimensions) w->Fit();if(maximized) w->Maximize(*maximized);if(fullscreenValue) w->ShowFullScreen(*fullscreenValue);w->sized=true;}
            w->setOpen(requested());ModalWindows::instance().refresh();
        }
        void destroy(bool direct) override {
            if(ui) {auto w=static_cast<ModalWindow *>(ui);w->setOpen(false);w->dismiss={};w->notifyOpen={};w->notifyClose={};}
            SingleChild::destroy(direct);
        }
    };
}}
namespace CPPUI {
    template<class T> inline Wx::Modal & Modal(Binding<T> value,T off=T{}) {return *new Wx::Modal(value,off);}
    template<class T> inline Wx::Modal & Modal(State<T> & value,T off=T{}) {return *new Wx::Modal(value,off);}
}
#endif

#ifdef CPPUI_TEST_QT
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include <QApplication>
#include <QTreeView>
#else
#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/label.hpp"
#endif
static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
class Root:public CPPUI::Application {
public:
    using Application::Application;
    bool show=true,handler=true,accept=true,remove=false;
    int calls=0,version=0,seen=-1,enters=0;
    CPPUI::DropEvent saved;
    void content() override {
        for(auto window:CPPUI::Window()) for(auto column:CPPUI::VBox()) if(show) {
            auto & label=CPPUI::Label("Drop here"); label.ref("drop");
            const int snapshot=version;
            if(handler) label.drop([this,snapshot](CPPUI::DropEvent & event) {
                ++calls; seen=snapshot; saved=event;
                if(accept) event.accept();
                if(remove) {show=false;sync();}
            }, {".txt", ".json"}).dragEnter([this](CPPUI::DropEvent &){++enters;});
        }
    }
};
#ifndef CPPUI_TEST_QT
class TestTarget:public CPPUI::Wx::DropTarget {
public:
    using DropTarget::DropTarget;
    bool GetData() override {return true;}
};
#endif
int main(int argc,char ** argv) {
    try {
        Root root(argc,argv); root.sync();
#ifdef CPPUI_TEST_QT
        auto native=static_cast<QWidget *>(CPPUI::ref("drop").node()->outer());
        QMimeData mime; mime.setText("Hello");
        mime.setUrls({QUrl::fromLocalFile("/tmp/FILE.TXT"),QUrl("https://example.com/item")});
        auto send=[&](Qt::DropAction action) {
            QDragEnterEvent enter(QPoint(2,3),action,&mime,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(native,&enter);
            QDropEvent event(QPointF(2,3),action,&mime,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(native,&event); return event.isAccepted();
        };
        for(auto action:{Qt::CopyAction,Qt::MoveAction,Qt::LinkAction}) {
            ++root.version; root.sync(); require(send(action),"native drop rejected accepted handler");
            require(root.seen==root.version && root.saved.files.size()==1 && root.saved.files[0].name=="FILE.TXT"
                && !root.saved.files[0].temporary && root.saved.urls.size()==1 && root.saved.x.value()==2,"native payload or handler was stale");
        }
        {
            QTreeView tree; CPPUI::Qt::DropFilter filter(&tree); filter.handlers.dropped=[](CPPUI::DropEvent & event){event.accept();}; filter.enable(true);
            QDragEnterEvent enter(QPoint(2,3),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(tree.viewport(),&enter);
            QDropEvent event(QPointF(2,3),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
            QApplication::sendEvent(tree.viewport(),&event);
            require(event.isAccepted(),"scroll-area viewport did not deliver portable drops");
        }
        require(root.saved.action=="link" && root.enters==3,"drop actions or enter callback failed");
        root.accept=false; root.sync(); require(!send(Qt::CopyAction),"unaccepted drop succeeded");
        root.handler=false; root.sync(); require(!native->acceptDrops(),"removed drop handler stayed registered");
        root.handler=true; root.accept=true; root.remove=true; root.sync();
        require(send(Qt::CopyAction),"self-removing drop did not finish");
        require(CPPUI::detail::refRegistry().count("drop")==0,"self-removal retained drop target");
#else
        auto native=static_cast<wxWindow *>(CPPUI::ref("drop").node()->outer());
        require(dynamic_cast<CPPUI::Wx::DropTarget *>(native->GetDropTarget()),"wx drop registration missing");
        auto target=new TestTarget(native); target->handlers=CPPUI::ref("drop").node()->dropHandlers;
        native->SetDropTarget(target);
        wxTextDataObject text("Hello"); std::vector<char> bytes(text.GetDataSize()); text.GetDataHere(bytes.data());
        require(target->data->SetData(text.GetFormat(),bytes.size(),bytes.data()),"wx text negotiation failed");
        for(auto action:{wxDragCopy,wxDragMove,wxDragLink}) require(target->OnData(2,3,action)==action,"wx accepted drop failed");
        require(root.calls==3 && root.saved.text.value()=="Hello" && root.saved.action=="link","wx payload or action lost");
        const std::string uris="#comment\r\nhttps://example.com/item\r\nhttps://example.com/second\n";
        target->uris->SetData(uris.size(),uris.data());
        auto event=target->eventData(2,3,wxDragCopy,target->uris->GetFormat());
        require(event.urls.size()==2 && event.urls[0]=="https://example.com/item","wx URI list parsing failed");
        target->files->AddFile("/tmp/FILE.TXT"); event=target->eventData(2,3,wxDragMove,target->files->GetFormat());
        require(event.files.size()==1 && event.files[0].name=="FILE.TXT" && !event.files[0].temporary,"wx file data lost");
        root.accept=false; root.version=1; root.sync();
        require(target->OnData(2,3,wxDragCopy)==wxDragNone && root.seen==1,"wx handler update/rejection failed");
        root.handler=false; root.sync(); require(!native->GetDropTarget(),"wx omitted handler stayed registered");
        root.handler=true; root.accept=true; root.remove=true; root.sync();
        auto current=static_cast<CPPUI::Wx::DropTarget *>(native->GetDropTarget());
        event=CPPUI::DropEvent(); event.text=std::string("Remove");
        require(current->dispatchData(event,wxDragCopy)==wxDragCopy,"self-removing wx drop failed");
        require(CPPUI::detail::refRegistry().count("drop")==0,"wx removal retained drop node");
#endif
    } catch(const std::exception & error) { std::cerr<<error.what()<<'\n'; return 1; }
}

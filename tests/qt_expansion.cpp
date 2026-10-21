#include "cppui/qt/backend.hpp"
#include <QApplication>
#include <QScrollBar>
static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
static CPPUI::Node * find(CPPUI::Node & root,const std::string & id) {
    if(root._id==id) return &root;
    for(auto child:*root.children) if(auto result=find(*child,id)) return result;
    return nullptr;
}
static void pump() {
    for(int i=0;i<12;++i) {QCoreApplication::processEvents();QCoreApplication::sendPostedEvents(nullptr,QEvent::LayoutRequest);}
}
struct Fragment:CPPUI::Qt::QtView {
    bool canvas=false;
    void content() override {
        if(canvas) CPPUI::Canvas([](CPPUI::CanvasPainter &) {return false;}).layout(40,40).id("dynamic");
        else CPPUI::Label("Static").id("dynamic");
    }
};
struct Root:CPPUI::Qt::QtView {
    bool horizontal=false,inverse=false,canvas=true,spacer=false,overridePolicy=false;
    CPPUI::Optional<int> weight;
    int mode=0;
    void content() override {
        for(auto window:CPPUI::Window().id("window")) {
            CPPUI::Node & box=horizontal ? static_cast<CPPUI::Node &>(CPPUI::HBox(inverse)) : CPPUI::VBox(inverse);
            for(auto scope:box.layout(CPPUI::layout::padding(0)).id("box")) {
                if(mode==0) {
                    CPPUI::Label("First").id("first");CPPUI::Divider();CPPUI::Label("Last").id("last");
                } else if(mode==1) {
                    CPPUI::Node & along=horizontal ? static_cast<CPPUI::Node &>(CPPUI::HBox()) : CPPUI::VBox();
                    for(auto nested:along.id("along").layout(CPPUI::layout::padding(0))) CPPUI::Label("Along");
                    CPPUI::Node & across=horizontal ? static_cast<CPPUI::Node &>(CPPUI::VBox()) : CPPUI::HBox();
                    for(auto nested:across.id("across").layout(CPPUI::layout::padding(0))) CPPUI::Label("Across");
                    CPPUI::Canvas([](CPPUI::CanvasPainter &) {return false;}).layout(40,40,weight).visible(canvas).id("canvas");
                    CPPUI::Spacer().visible(spacer).id("spacer");
                } else if(mode==2) {
                    CPPUI::Label("Weighted").layout({}, {},weight).id("label");CPPUI::Spacer().id("spacer");
                } else if(mode==3) {
                    for(auto nested:CPPUI::VBox().id("sibling")) CPPUI::Label("Sibling");
                    CPPUI::SubView<Fragment>().id("fragment");
                } else if(mode==4) {
                    auto & label=CPPUI::Label("Override");label.id("label");
                    if(overridePolicy) {CPPUI::QtOptions options;options.HorizontalPolicy=QSizePolicy::Expanding;label.qt(options);}
                } else {
                    CPPUI::Label("Heading");
                    for(auto scroll:CPPUI::Scroll().layout(CPPUI::layout::weight(1)).id("scroll"))
                        for(auto rows:CPPUI::VBox().layout(CPPUI::layout::padding(0)).id("rows"))
                            for(int i=0;i<80;++i) CPPUI::Label(std::to_string(i));
                }
            }
        }
    }
    void mount() {
        sync();auto window=static_cast<QWidget *>(find(*this,"window")->outer());window->resize(800,600);window->show();pump();
    }
    QWidget * widget(const char * id) {return static_cast<QWidget *>(find(*this,id)->outer());}
    int extent(const char * id) {
        auto node=find(*this,id);
        auto spacer=dynamic_cast<CPPUI::Qt::Spacer *>(node);
        auto rect=spacer ? spacer->ui->geometry() : static_cast<QWidget *>(node->outer())->geometry();
        return horizontal ? rect.width() : rect.height();
    }
};
int main(int argc,char ** argv) {
    QApplication app(argc,argv);
    try {
        Root root;
        for(bool horizontal:{false,true}) for(bool inverse:{false,true}) {
            root.horizontal=horizontal;root.inverse=inverse;root.mount();
            for(auto id:{"first","last"}) require(root.widget(id)->size()==root.widget(id)->sizeHint(),"static label expanded beyond its hint");
            const auto first=root.widget("first")->pos(),last=root.widget("last")->pos();
            require((horizontal ? first.x()>last.x() : first.y()>last.y())==inverse,"static reverse order wrong");
            require((horizontal ? std::min(first.x(),last.x()) : std::min(first.y(),last.y()))==0,"static labels not packed at the start");
        }
        root.mode=1;root.inverse=false;
        for(bool horizontal:{false,true}) {
            root.horizontal=horizontal;
            for(int state=0;state<5;++state) {
                root.canvas=state!=1;root.spacer=state==2;root.weight=state==3 ? CPPUI::Optional<int>(0) : CPPUI::Optional<int>();
                root.mount();const std::string winner=state==1 || state==3 ? "along" : state==2 ? "spacer" : "canvas";
                for(auto id:{"along","across","canvas","spacer"}) {
                    if(!find(root,id)->is_visible) continue;
                    require(id==winner ? root.extent(id)>300 : root.extent(id)<150,"expansion priority chose wrong winner");
                }
            }
        }
        root.mode=2;root.horizontal=false;
        for(int weight:{1,2,0,-1,1}) {
            root.weight=weight<0 ? CPPUI::Optional<int>() : CPPUI::Optional<int>(weight);root.mount();
            if(weight>0) require(std::abs(root.extent("label")-root.extent("spacer")*weight)<=2,"weighted static control did not share extra space");
            else require(root.widget("label")->height()==root.widget("label")->sizeHint().height(),"removed/zero weight did not restore natural height");
        }
        root.mode=3;root.mount();
        auto fragment=static_cast<Fragment *>(find(root,"fragment"));
        for(bool canvas:{true,false,true}) {
            fragment->canvas=canvas;fragment->sync();pump();
            require(canvas ? root.extent("dynamic")>500 && root.extent("sibling")<80 : root.extent("sibling")>500,
                "independent view failed to recalculate sibling expansion");
        }
        delete root.widget("sibling");
        fragment->canvas=false;fragment->sync();pump();
        require(root.widget("dynamic")->height()==root.widget("dynamic")->sizeHint().height(),
            "external native deletion left stale sibling expansion");
        root.mode=4;root.horizontal=true;
        for(bool overridePolicy:{true,false,true}) {
            root.overridePolicy=overridePolicy;root.mount();
            require(overridePolicy ? root.extent("label")==800 : root.widget("label")->width()==root.widget("label")->sizeHint().width(),"native policy override did not apply/reset");
        }
        root.mode=5;root.horizontal=false;root.mount();
        auto scroll=static_cast<QScrollArea *>(root.widget("scroll"));
        require(scroll->verticalScrollBar()->maximum()>500,"scroll lost natural content height");
        for(auto row:*find(root,"rows")->children) {
            auto widget=static_cast<QWidget *>(row->outer());require(widget->height()==widget->sizeHint().height(),"scroll rows grew vertically");
        }
        std::cout<<"Qt expansion passed\n";
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

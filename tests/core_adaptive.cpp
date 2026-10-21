#include "cppui/state.hpp"
#include <stdexcept>

static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
static int starts=0,stops=0;
struct Observer:CPPUI::detail::SizeObserver {
    Observer() {++starts;}
    ~Observer() override {++stops;}
    void refresh() override {}
};
struct Node:CPPUI::Node {
    void update(CPPUI::Node * previous) override {ui=previous ? previous->ui : this;CPPUI::Node::update(previous);}
    std::unique_ptr<CPPUI::detail::SizeObserver> observeSize() override {
        return std::unique_ptr<CPPUI::detail::SizeObserver>(new Observer());
    }
    void destroy(bool) override {}
};
struct Box:Node {
    bool horizontal;
    Box(int threshold,bool width):horizontal(CPPUI::detail::adaptiveHorizontal(*this,threshold,width)) {}
};
static Node & node() {return *(new Node());}
struct Root:CPPUI::View {
    std::function<void()> build;
    int builds=0;
    ~Root() {for(auto child:*children) CPPUI::recur_delete(child,true);children->clear();}
    void content() override {++builds;build();}
    void destroy(bool) override {}
};
int main() {
    try {
        CPPUI::VisibilityBounds bounds(100,200,50,100);
        require(!bounds.matches({}) && bounds.matches(CPPUI::Size(100,50)) && !bounds.matches(CPPUI::Size(200,50))
            && !bounds.matches(CPPUI::Size(100,100)) && !bounds.matches(CPPUI::Size(99,60)),"breakpoint boundaries differ from PUI");
        {
            namespace layout=CPPUI::layout;
            namespace style=CPPUI::style;
            Node orphan;
            require(&orphan.wideLayout(0,layout::width(200)).wideStyle(0,style::color(0))==&orphan
                && !orphan.layout_width && !orphan.style_color,"parentless overrides should not match");
            Root root;bool present=true;
            const char * family="Sans";
            root.build=[&]() {
                if(!present) return;
                for(auto container:node()) {
                    auto & wrapper=node();wrapper.is_virtual=true;
                    for(auto scope:wrapper) {
                        node().layout(layout::width(100),layout::padding(2))
                            .style(style::color(0x111111),style::fontSize(12),style::fontWeight(CPPUI::FontWeight::Normal))
                            .wideLayout(600,layout::width(200),layout::weight(0),layout::padding(8))
                            .wideStyle(600,style::color(0),style::fontSize(18),style::fontFamily(family),style::fontWeight(CPPUI::FontWeight::DemiBold))
                            .wideStyle(800,{}, {}, {},CPPUI::FontWeight::Bold)
                            .wideLayout(800,300).wideLayout(600,{},30)
                            .wideLayout(800,layout::height(40)).wideLayout(600,layout::height(35))
                            .wideStyle(800,0x222222).wideStyle(600,0)
                            .wideStyle(600,style::fontFamily(nullptr));
                    }
                }
            };
            root.redraw();
            auto state=root.children->front()->sizeState;
            auto child=[&]() {return root.children->front()->children->front()->children->front();};
            require(state && *child()->layout_width==100 && !child()->layout_height &&
                    *child()->style_fontsize==12 && *child()->style_fontweight==400 && !child()->style_fontfamily,"unknown width matched an override");
            auto identity=child()->ui;
            for(int width:{599,600,800,400,600}) {
                state->publish(width,400);
                auto current=child();const bool wide=width>=600;
                require(current->ui==identity && *current->layout_width==(width>=800 ? 300 : wide ? 200 : 100),
                        "wide layout changed identity or selected the wrong threshold");
                require(bool(current->layout_height)==wide && (!wide || *current->layout_height==35) &&
                        bool(current->layout_weight)==wide && (!wide || *current->layout_weight==0) &&
                        current->layout_padding->left==(wide ? 8 : 2),"wide layout merge/restore/order failed");
                require(*current->style_color==(wide ? 0 : 0x111111) &&
                        *current->style_fontsize==(wide ? 18 : 12) &&
                        *current->style_fontweight==(width>=800 ? 700 : wide ? 600 : 400) &&
                        current->style_fontfamily==(wide ? family : nullptr),"wide style merge/borrow/restore/order failed");
                require(!root.children->front()->children->front()->sizeState,"wide override observed a virtual parent");
            }
            int before=root.builds;state->publish(600,400);
            require(root.builds==before,"stable width rebuilt conditional modifiers");
            present=false;root.redraw();before=root.builds;state->publish(900,400);
            require(root.builds==before,"removed wide override kept its size subscription");
        }
        {
            Root root;bool remove=false,fail=false;CPPUI::State<int> external(0);
            const int initialStarts=starts,initialStops=stops;
            root.build=[&]() {
                if(remove) return;
                auto & container=node();container.id("container");container.size().get();
                for(auto scope:container) {
                    for(auto compact:node().id("compact").visible(true,{{},600})) node();
                    for(auto wide:node().id("wide").visible(true,{600})) node();
                }
                if(fail) external=1;
            };
            root.redraw();auto container=root.children->front();auto state=container->sizeState;auto binding=container->size();
            require(!binding.get() && container->children->front()->children->empty(),"unmeasured branch mounted content");
            auto callback=[state](int w,int h) {state->publish(w,h);};
            callback(700,300);container=root.children->front();
            require(container->sizeState==state && starts==initialStarts+1 && stops==initialStops,"size observer/state not retained");
            require(!container->children->front()->is_visible && container->children->back()->is_visible,"wide breakpoint failed");
            auto identity=container->children->back()->children->front()->ui;
            callback(599,300);require(root.children->front()->children->front()->is_visible,"compact breakpoint failed");
            callback(600,300);container=root.children->front();
            require(container->children->back()->is_visible && container->children->back()->children->front()->ui==identity,"boundary show lost identity");
            int before=root.builds;callback(600,300);require(root.builds==before,"same geometry caused redundant redraw");
            require(binding.get()->width==600,"saved binding stopped following retained declarations");
            fail=true;
            try {root.redraw();require(false,"mutation guard absent");}catch(const CPPUI::StateMutationInViewBuilderError &) {}
            fail=false;callback(500,300);
            require(root.children->front()->children->front()->is_visible,"failed build detached the mounted size state");
            callback(-20,-5);require(binding.get()->width==0 && binding.get()->height==0,"negative size was not clamped");
            remove=true;root.redraw();before=root.builds;callback(800,300);
            require(root.builds==before && stops==initialStops+1,"removed observer continued publishing or failed cleanup");
        }
        {
            Root root;bool present=true;
            root.build=[&]() {
                if(!present) return;
                for(auto container:node()) {
                    auto & wrapper=node();wrapper.is_virtual=true;
                    for(auto scope:wrapper) {
                        for(auto box:(new Box(600,true))->id("width")) node();
                        for(auto box:(new Box(400,false))->id("height")) node();
                    }
                }
            };
            root.redraw();auto state=root.children->front()->sizeState;
            auto boxes=[&]() {return root.children->front()->children->front()->children;};
            require(!static_cast<Box *>(boxes()->front())->horizontal && static_cast<Box *>(boxes()->back())->horizontal,
                "unknown size did not choose the fallback direction");
            auto first=boxes()->front()->ui,second=boxes()->back()->ui,editor=boxes()->front()->children->front()->ui;
            for(auto size:{CPPUI::Size(600,400),CPPUI::Size(599,399),CPPUI::Size(900,700),CPPUI::Size(400,200)}) {
                state->publish(size.width,size.height);
                require(static_cast<Box *>(boxes()->front())->horizontal==(size.width>=600) &&
                        static_cast<Box *>(boxes()->back())->horizontal==(size.height<400),"adaptive box breakpoint failed");
                require(boxes()->front()->ui==first && boxes()->back()->ui==second &&
                        boxes()->front()->children->front()->ui==editor,"direction change replaced retained nodes");
                require(!root.children->front()->children->front()->sizeState,"adaptive box observed a virtual ancestor");
            }
            int before=root.builds;state->publish(400,200);require(root.builds==before,"stable box geometry rebuilt");
            present=false;root.redraw();before=root.builds;state->publish(900,700);
            require(root.builds==before,"removed adaptive box remained subscribed");
        }
        {
            Root root;bool reverse=false;
            root.build=[&]() {
                node().size().get();node().size().get();
                node().id(reverse ? "b" : "a").size().get();node().id(reverse ? "a" : "b").size().get();
            };
            root.redraw();
            auto first=root.children->at(0)->sizeState,second=root.children->at(1)->sizeState;
            auto a=root.children->at(2)->sizeState,b=root.children->at(3)->sizeState;
            first->publish(100,20);second->publish(200,20);a->publish(300,20);b->publish(400,20);
            reverse=true;root.redraw();
            require(root.children->at(0)->sizeState==first && root.children->at(1)->sizeState==second,"anonymous siblings shared size state");
            require(root.children->at(2)->sizeState==b && root.children->at(3)->sizeState==a,"keyed reorder swapped size state");
        }
        {
            struct Prefix:Node {};
            Root root;bool prefix=true;std::vector<int> read;
            root.build=[&]() {
                if(prefix) new Prefix();
                read.clear();
                for(int i=0;i<2;++i) {auto size=node().size().get();read.push_back(size ? size->width : -1);}
            };
            root.redraw();
            root.children->at(1)->sizeState->publish(100,20);
            root.children->at(2)->sizeState->publish(200,20);
            prefix=false;root.redraw();
            require(read==std::vector<int>({100,200}),"unrelated sibling removal misbound anonymous sizes during construction");
        }
        require(starts==stops,"size observer leaked");
    }catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

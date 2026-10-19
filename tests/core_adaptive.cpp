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
            Root root;bool remove=false,fail=false;CPPUI::State<int> external(0);
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
            require(container->sizeState==state && starts==1 && stops==0,"size observer/state not retained");
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
            require(root.builds==before && stops==1,"removed observer continued publishing or failed cleanup");
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

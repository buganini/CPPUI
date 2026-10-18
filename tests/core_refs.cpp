#include "cppui/view.hpp"
#include <thread>
#include <stdexcept>
static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
static std::vector<std::function<void()>> pending;
static CPPUI::Node * applied=nullptr;
class Item: public CPPUI::Node {
public:
    void destroy(bool) override { clearRef(); }
    bool refSelection() const override { return true; }
    CPPUI::CommandPoster commandPoster() override { return [](std::function<void()> task){pending.push_back(task);}; }
    void refCommand(CPPUI::RefCommand,const CPPUI::OptionalText &,bool) override { applied=this; }
};
class Root: public CPPUI::View {
public:
    bool show=true, duplicate=false;
    std::string name="target", identity="item";
    ~Root() override { for(auto child:*children) CPPUI::recur_delete(child,true); children->clear(); }
    void destroy(bool) override {}
    void content() override {
        if(show) (new Item())->id(identity).ref(name);
        if(duplicate) (new Item())->id("second").ref(name);
    }
};
static void flush() { auto tasks=std::move(pending); pending.clear(); for(auto & task:tasks) task(); }
int main() {
    Root root; root.redraw(); auto old=CPPUI::ref("target");
    std::thread worker([old](){old.select("page");}); worker.join();
    require(!applied,"worker command was applied synchronously");
    root.redraw(); flush(); require(applied==root.children->front() && old.node()==applied,"binder did not follow reconciliation");
    bool invalid=false; try { old.select(""); } catch(const std::invalid_argument &) { invalid=true; }
    require(invalid,"invalid selection was queued");
    old.select("page"); root.identity="replacement"; root.redraw(); applied=nullptr; flush();
    require(!applied && !old.node(),"old binder targeted a replacement");
    bool expired=false; try { old.select("page"); } catch(const std::runtime_error &) { expired=true; }
    require(expired,"destroyed binder did not fail");
    auto replacement=CPPUI::ref("target"); root.name="renamed"; root.redraw();
    require(!replacement.node() && CPPUI::detail::refRegistry().count("target")==0,"renamed ref leaked old registration");
    root.duplicate=true; root.redraw(); auto second=CPPUI::ref("renamed");
    root.show=false; root.redraw(); require(CPPUI::ref("renamed").node()==second.node(),"old duplicate erased current ref");
    second.select("page"); root.duplicate=false; root.redraw(); applied=nullptr; flush();
    require(!applied && !second.node() && CPPUI::detail::refRegistry().empty(),"removed ref leaked a node or dispatched");
    class Nested:public CPPUI::View {
        public:
        bool ancestorVisible=false;
        void destroy(bool) override {}
        void content() override {
            ancestorVisible=CPPUI::ref("nested").node()==parent;
            (new Item())->ref("nested");
        }
    };
    class NestedRoot:public CPPUI::View {
        public:
        Nested * nested=nullptr;
        ~NestedRoot() override {for(auto node:*children) CPPUI::recur_delete(node,true);children->clear();}
        void destroy(bool) override {}
        void content() override {
            for(auto scope:(new Item())->ref("nested")) nested=new Nested();
        }
    };
    {
        NestedRoot nested; nested.redraw();
        require(nested.nested->ancestorVisible,"ancestor ref was not published before descendant content");
        require(CPPUI::ref("nested").node()==nested.nested->children->front(),"parent publication overwrote nested duplicate");
    }
    require(CPPUI::detail::refRegistry().empty(),"nested refs leaked registry entries");

}

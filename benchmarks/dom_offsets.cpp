// g++ -O3 -DNDEBUG -std=c++11 -I. benchmarks/dom_offsets.cpp -o /tmp/dom_offsets
// No toolkit or drawing cost: measures repeated independent retained-view updates.
#include "cppui/view.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>

namespace {
    struct Leaf: CPPUI::Node {void destroy(bool) override {}};
    struct Fragment: CPPUI::View {
        int & width;
        explicit Fragment(int & width):width(width) {}
        void content() override {for(int i=0;i<width;++i) (new Leaf)->id(std::to_string(i));}
        void destroy(bool) override {}
    };
    struct Root: CPPUI::View {
        int count,width=3;
        explicit Root(int count):count(count) {}
        ~Root() override {for(auto node:*children) CPPUI::recur_delete(node,true);children->clear();}
        void content() override {for(auto parent:*new Leaf) for(int i=0;i<count;++i) (new Fragment(width))->id(std::to_string(i));}
        void destroy(bool) override {}
    };
    volatile int sink=0;
    template<class Fn> double time(int count,Fn fn) {
        const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<count;++i) fn(i);
        return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count()/count;
    }
}
int main() {
    std::cout<<"views,lookup_us,stable_redraw_us,structural_redraw_us\n"<<std::fixed<<std::setprecision(4);
    for(int count:{16,128,1024}) {
        Root root(count);root.redraw();auto parent=root.children->front();
        std::vector<Fragment *> targets;for(auto child:*parent->children) targets.push_back(static_cast<Fragment *>(child));
        auto lookup=[&](int i){int offset=0;parent->findDomOffsetForNode(targets[i%count],&offset);sink=offset;};
        auto update=[&](int i){targets[i%count]->redraw();};
        for(int i=0;i<count;++i){lookup(i);update(i);}
        double l=time(50000,lookup),u=time(20000,update);
        double s=time(2000,[&](int i){root.width=2+(i/count)%2;targets[i%count]->redraw();});
        std::cout<<count<<','<<l<<','<<u<<','<<s<<'\n';
    }
}

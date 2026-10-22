#include "cppui/grid.hpp"
#include <sstream>

static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
struct Item:CPPUI::Node {void destroy(bool) override {}};
struct Grid:CPPUI::detail::GridLayout<Item> {};
struct Capture {
    std::ostringstream output;
    std::streambuf * previous=std::cerr.rdbuf(output.rdbuf());
    ~Capture() {std::cerr.rdbuf(previous);}
    std::string take() {auto text=output.str();output.str("");return text;}
};
static void declare(int threshold) {
    Grid node;
    node.wideStyle(threshold,CPPUI::style::fontSize(20))
        .wideStyle(threshold-1,CPPUI::style::fontSize(16));
}
int main() {
    try {
        Capture log;
        const char * names[]={"wideLayout","wideStyle","wideVisible","wideGrid","wideColumns"};
        for(int method=0;method<5;++method) {
            Grid node;
            for(int width:{100,50}) switch(method) {
                case 0:node.wideLayout(width,CPPUI::layout::width(20));break;
                case 1:node.wideStyle(width,{}, {}, {},CPPUI::FontWeight::Bold);break;
                case 2:node.wideVisible(width,false);break;
                case 3:node.wideGrid(width,CPPUI::grid::columnspan(2));break;
                case 4:node.wideColumns(width,2);break;
            }
            const auto warning=log.take();
            require(warning.find(std::string(names[method])+" breakpoint 50 follows 100")!=std::string::npos,
                "descending unmatched breakpoint did not warn");
            if(CPPUI::detail::Breakpoint(0).line)
                require(warning.find("core_wide_order.cpp:")!=std::string::npos,"warning lost declaration location");
            require(warning.find("node_header.hpp")==std::string::npos,"forwarded overload reported an internal location");
        }
        {
            Grid node;
            node.wideStyle(100).wideStyle(100).wideStyle(200);
            node.wideLayout(10).wideVisible(1,true).wideGrid(0,CPPUI::grid::row(0));
            node.wideColumns(5,2);
            require(log.take().empty(),"equal/ascending or independent methods warned");
        }
        declare(7000);require(!log.take().empty(),"first declaration did not warn");
        declare(7000);require(log.take().empty(),"rebuilding a declaration repeated its warning");
        for(int width=8000;width<8127;++width) declare(width);
        log.take();declare(7000);
        require(log.take().empty(),"warning cache evicted before 128 entries");
        for(int width=9000;width<9128;++width) declare(width);
        log.take();declare(7000);
        require(!log.take().empty(),"warning cache did not evict beyond 128 entries");
        Item parent;parent.update(nullptr);parent.size();parent.sizeState->publish(1000,100);
        {
            Item node;node.parent=&parent;
            node.wideStyle(800,CPPUI::style::fontSize(20)).wideStyle(600,CPPUI::style::fontSize(16));
            require(*node.style_fontsize==16,"diagnostics reordered matching declarations");
        }
    }catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

#include "cppui/file_filter.hpp"
#include "cppui/url.hpp"
#include "cppui/color.hpp"
#include "cppui/path.hpp"
#include "cppui/rgba.hpp"
#include <limits>
#include <iostream>
static void require(bool value) {if(!value) throw std::runtime_error("shared PUI contract");}
template<class F> static void invalid(F f) {bool thrown=false;try{f();}catch(const std::invalid_argument &){thrown=true;}require(thrown);}
int main() {
    CPPUI::FileFilters flat{".txt","cpp","txt"};
    require(flat.native(true)=="Files (*.txt *.cpp)" && flat.native(false)=="Files|*.txt;*.cpp");
    CPPUI::FileFilters grouped(std::vector<CPPUI::FileFilterGroup>{{"Text",{"txt"}},{"All",{"*"}}});
    require(grouped.native(true)=="Text (*.txt);;All (*)");require(CPPUI::FileFilters().native(false).empty());
    for(auto extension:{"","*.txt","x y","../a","txt;cpp"}) invalid([=](){CPPUI::FileFilters f{extension};});
    invalid([](){CPPUI::FileFilters f(std::vector<CPPUI::FileFilterGroup>{{"Empty",{}}});});
    for(auto url:{"https://example.org/path?q=1","http://localhost:8766","mailto:a@example.org?subject=Hello"}) require(CPPUI::detail::validateURL(url)==url);
    for(auto url:{"","example.org","file:///tmp/a","javascript:alert(1)","https:///path","https://","mailto:"," https://example.org","https://example.org\n"}) invalid([=](){CPPUI::detail::validateURL(url);});
    require(CPPUI::Color(0x123456)==0x123456 && CPPUI::Color(0x123456,0)==0xff123456 && CPPUI::Color(0x123456,.5)==0x80123456);
    invalid([](){CPPUI::Color(0x1000000);});invalid([](){CPPUI::Color(0,-.1);});invalid([](){CPPUI::Color(0,std::numeric_limits<double>::quiet_NaN());});
    CPPUI::detail::validatePath({{CPPUI::PathOp::MoveTo,{0,0}},{CPPUI::PathOp::CubicTo,{0,1,1,0,1,1}},{CPPUI::PathOp::Close}},CPPUI::FillRule::EvenOdd);
    invalid([](){CPPUI::detail::validatePath({{CPPUI::PathOp::LineTo,{0,0}}},CPPUI::FillRule::NonZero);});
    invalid([](){CPPUI::detail::validatePath({{CPPUI::PathOp::MoveTo,{0}}},CPPUI::FillRule::NonZero);});
    CPPUI::detail::validateRGBA(1,1,{0,0,0,255});invalid([](){CPPUI::detail::validateRGBA(1,1,{0,0,0});});
}

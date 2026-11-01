#ifndef CPPUI_TEST_CANVAS_PATHS
#define CPPUI_TEST_CANVAS_PATHS
#include "cppui/path.hpp"
template<class Painter> void pathFixture(Painter & c) {
    using CPPUI::PathOp;
    c.drawRect(0,0,64,64,0xffffff);
    c.push();c.clipRect(4,4,20,20);
    c.drawPath({{PathOp::MoveTo,{0,0}},{PathOp::LineTo,{40,0}},{PathOp::LineTo,{40,40}},{PathOp::LineTo,{0,40}},{PathOp::Close}},0xff0000);
    c.pop();c.drawRect(30,4,40,14,0x00ff00);
    c.drawRect(4,30,20,46,CPPUI::Color(0x0000ff,.5));
    c.drawPath({{PathOp::MoveTo,{30,30}},{PathOp::LineTo,{60,30}},{PathOp::LineTo,{60,60}},{PathOp::LineTo,{30,60}},{PathOp::Close},
        {PathOp::MoveTo,{40,40}},{PathOp::LineTo,{50,40}},{PathOp::LineTo,{50,50}},{PathOp::LineTo,{40,50}},{PathOp::Close}},0x000000,{},1,CPPUI::FillRule::EvenOdd);
}
#endif

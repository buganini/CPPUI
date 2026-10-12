#ifndef CPPUI_WX_TIMELINE
#define CPPUI_WX_TIMELINE
#include "base.hpp"
#include <wx/timer.h>
#include <cmath>
#include <limits>
namespace CPPUI { namespace Wx {
class TimelineView: public ::CPPUI::Node {
    class Timer: public wxTimer { View * root; public: explicit Timer(View * root):root(root){} void Notify() override {root->redraw();} };
    std::unique_ptr<Timer> timer;
    int milliseconds;
public:
    explicit TimelineView(double seconds) {
        if(!std::isfinite(seconds)||seconds<0||seconds*1000>std::numeric_limits<int>::max())throw std::invalid_argument("Timeline interval must be finite, nonnegative and fit wx milliseconds");
        if(!dynamic_cast<View *>(cppui_root))throw std::logic_error("TimelineView requires a wx View");
        milliseconds=std::max(1,static_cast<int>(std::ceil(seconds*1000)));is_virtual=true;
    }
    void update(::CPPUI::Node * previous) override {
        auto prev=static_cast<TimelineView *>(previous);if(prev)timer=std::move(prev->timer);
        if(!timer){timer.reset(new Timer(static_cast<View *>(cppui_root)));timer->Start(milliseconds);}
        ::CPPUI::Node::update(previous);
    }
    void destroy(bool) override {timer.reset();}
};
}
inline Wx::TimelineView & TimelineView(double seconds){return *new Wx::TimelineView(seconds);}
}
#endif

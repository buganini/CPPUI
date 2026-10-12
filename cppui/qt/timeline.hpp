#ifndef CPPUI_QT_TIMELINE
#define CPPUI_QT_TIMELINE

#include "base.hpp"
#include <QTimer>
#include <cmath>
#include <limits>
#include <memory>

namespace CPPUI { namespace Qt {
    class TimelineView: public Node {
        int milliseconds;
        std::unique_ptr<QTimer> timer;
        public:
        explicit TimelineView(double ttlSeconds) {
            if(!std::isfinite(ttlSeconds) || ttlSeconds < 0
                || ttlSeconds * 1000 > std::numeric_limits<int>::max()) {
                throw std::invalid_argument("TimelineView interval must be finite, nonnegative and fit Qt milliseconds");
            }
            if(!dynamic_cast<QtView *>(cppui_root)) throw std::logic_error("TimelineView requires a Qt Application or QtView");
            milliseconds = static_cast<int>(std::ceil(ttlSeconds * 1000));
            is_virtual = true;
        }
        void update(Node * previous) override {
            auto prev = static_cast<TimelineView *>(previous);
            if(prev) timer = std::move(prev->timer);
            if(!timer) {
                timer.reset(new QTimer());
                auto root = static_cast<QtView *>(cppui_root);
                QObject::connect(timer.get(), &QTimer::timeout, timer.get(), [root]() { root->redraw(); });
                timer->start(milliseconds);
            }
            Node::update(previous);
        }
        void destroy(bool) override { timer.reset(); }
    };
}}
namespace CPPUI {
    inline Qt::TimelineView & TimelineView(double ttlSeconds) { return *new Qt::TimelineView(ttlSeconds); }
}
#endif

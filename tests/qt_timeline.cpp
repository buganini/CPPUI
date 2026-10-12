#include "cppui/qt/timeline.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include <QApplication>
#include <QElapsedTimer>
#include <QEvent>
#include <QThread>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void pump(int ms) {
        QElapsedTimer elapsed; elapsed.start();
        do { QCoreApplication::processEvents(); QThread::msleep(1); } while(elapsed.elapsed() < ms);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    int declarations = 0, builds = 0;
    class Timeline: public CPPUI::Qt::TimelineView {
        public:
        explicit Timeline(double seconds): TimelineView(seconds) { ++declarations; }
        ~Timeline() override { --declarations; }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        bool show = true, reverse = false;
        double seconds = 0.01;
        void content() override {
            ++builds;
            for(auto window: CPPUI::Window()) for(auto row: CPPUI::HBox()) {
                if(reverse) CPPUI::Label("tail").id("tail");
                if(show) for(auto timeline: (new Timeline(seconds))->id("timer")) CPPUI::Label(std::to_string(builds));
                if(!reverse) CPPUI::Label("tail").id("tail");
            }
        }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root; root.sync();
            auto row = static_cast<QWidget *>(root.children->front()->children->front()->outer());
            auto label = static_cast<QLabel *>(row->layout()->itemAt(0)->widget());
            pump(80);
            require(builds > 2 && declarations == 1 && label == row->layout()->itemAt(0)->widget(), "timer did not redraw/reuse or retained retired declarations");
            root.reverse = true; root.sync();
            const auto before = builds;
            // PUI retains the mounted timer/interval when its declaration changes.
            root.seconds = 60; root.sync(); pump(80);
            require(builds > before + 1 && row->layout()->itemAt(1)->widget() == label, "keyed timer move replaced its interval or lost native order");
            root.show = false; root.sync(); pump(40);
            const auto stopped = builds;
            pump(40);
            require(builds == stopped && declarations == 0, "removed timeline kept firing");
            for(int i = 0; i < 20; ++i) {
                root.show = true; root.seconds = 0; root.sync();
                root.show = false; root.sync();
            }
            pump(10);
            require(declarations == 0, "rapid mount/removal leaked timers");
            root.show = true; root.seconds = -1;
            bool failed = false;
            try { root.sync(); } catch(const std::invalid_argument &) { failed = true; }
            require(failed && declarations == 0, "invalid interval leaked a declaration");
            root.seconds = 0.001; root.sync();
            root.redraw();
        }
        const auto stopped = builds;
        pump(40);
        require(builds == stopped && declarations == 0 && qt.topLevelWidgets().isEmpty(), "root teardown left queued timer work or native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

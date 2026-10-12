#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/state.hpp"
#include <QTimer>
#include <sstream>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        for(int i = 0; i < 4; ++i) QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    class Child: public CPPUI::Qt::QtView {
        CPPUI::State<int> & value;
        int & setups, & builds;
        public:
        Child(CPPUI::State<int> & value, int & setups, int & builds): value(value), setups(setups), builds(builds) {}
        void setup() override { ++setups; value = 7; }
        void content() override { ++builds; CPPUI::Label(std::to_string(static_cast<int>(value))); }
    };
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<int> value{0};
        bool show = false, fail = false, again = false;
        int setups = 0, builds = 0, childSetups = 0, childBuilds = 0;
        using Application::Application;
        void setup() override { ++setups; value = 1; }
        void content() override {
            ++builds;
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) {
                CPPUI::Label(std::to_string(static_cast<int>(value)));
                if(show) (new Child(value, childSetups, childBuilds))->id("child");
            }
            if(again) { again = false; redraw(); sync(); }
            if(fail) throw std::runtime_error("expected queued builder failure");
        }
        QLabel * label() { return static_cast<QLabel *>(children->front()->children->front()->children->front()->outer()); }
    };
    class Errors {
        std::streambuf * previous;
        public:
        std::ostringstream text;
        Errors() { previous = std::cerr.rdbuf(text.rdbuf()); }
        ~Errors() { std::cerr.rdbuf(previous); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    qt.setQuitOnLastWindowClosed(false);
    try {
        {
            Root root(argc, argv);
            root.redraw(); root.redraw();
            require(root.builds == 0, "redraw ran immediately");
            settle();
            require(root.builds == 1 && root.setups == 1 && root.label()->text() == "1", "initial redraw/setup was duplicated");
            auto native = root.label();
            for(int i = 2; i <= 10; ++i) root.value = i;
            require(root.builds == 1 && native->text() == "1", "state writes reconciled inside dispatch");
            settle();
            require(root.builds == 2 && native == root.label() && native->text() == "10", "state burst was not coalesced");
            root.value = 11;
            root.sync();
            settle();
            require(root.builds == 3 && native->text() == "11", "explicit sync left a duplicate queued redraw");
            root.again = true;
            root.redraw();
            settle();
            require(root.builds == 5, "redraw/sync during reconciliation nested or lost the follow-up");
            root.show = true;
            root.sync(); // Nested setup changes an already-subscribed parent's State.
            settle();
            require(root.childSetups == 1 && root.childBuilds == 1 && native->text() == "7", "nested setup was repeated or its state change was lost");
            const int childBuilds = root.childBuilds;
            root.value = 12; // Queue both parent and child, then remove the child first.
            root.show = false;
            root.sync();
            settle();
            require(root.childBuilds == childBuilds && native->text() == "12", "queued redraw reached a removed child");
            {
                Errors errors;
                root.fail = true;
                root.value = 13;
                settle();
                require(root.label() == native && native->text() == "12", "failed builder replaced the mounted tree");
                require(errors.text.str().find("expected queued builder failure") != std::string::npos, "queued failure was not reported");
                const int builds = root.builds;
                settle();
                require(root.builds == builds, "failed render retried forever");
                bool threw = false;
                try { root.sync(); } catch(const std::runtime_error &) { threw = true; }
                require(threw && root.label() == native, "immediate sync did not propagate its failure");
            }
            root.fail = false;
            root.value = 14;
            settle();
            require(native->text() == "14" && root.setups == 1, "failed view could not recover");
            root.redraw(); // Root destructor must cancel this request.
        }
        settle();
        {
            // run() initializes Qt and mounts immediately before the event loop.
            Root root(argc, argv);
            QTimer::singleShot(0, &qt, [&root]() { root.quit(); });
            root.run();
            require(root.builds == 1 && root.setups == 1, "run did not perform immediate startup");
        }
        settle();
        require(qt.topLevelWidgets().isEmpty(), "queued work retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

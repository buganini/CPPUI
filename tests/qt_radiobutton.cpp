#include "cppui/qt/radiobutton.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<int> first{1}, second{2};
        bool other = false, show = true;
        bool disable = false, setDisabled = true;
        int writes = 0, clicks = 0, version = 1;
        std::function<void()> action;
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) if(show) {
                auto state = other ? &second : &first;
                CPPUI::Binding<int> binding([state]() { return static_cast<int>(*state); }, [this, state](const int & value) {
                    ++writes; auto invoke = action; *state = value; if(invoke) invoke();
                });
                const auto current = version;
                auto & control = CPPUI::RadioButton("A", 1, binding);
                if(setDisabled) control.disabled(disable);
                control.click([this, current]() { clicks += current; });
                CPPUI::RadioButton("B", 2, binding).click([this, current]() { clicks += current; });
            }
        }
        QRadioButton * button(int index) { return static_cast<QRadioButton *>(children->front()->children->front()->children->at(index)->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync();
            auto first = root.button(0), second = root.button(1);
            require(first->isChecked() && !second->isChecked(), "initial value selection failed");
            root.disable = true; root.sync(); QCoreApplication::processEvents();
            require(!first->isEnabled(), "disabled control remained interactive");
            first->click();
            require(root.writes == 0 && root.clicks == 0, "disabled click wrote the model or invoked callback");
            root.first = 2; root.sync(); QCoreApplication::processEvents();
            require(bool(first->isChecked()) == false && root.writes == 0 && root.clicks == 0,
                "disabled selection did not follow model silently");
            root.disable = false; root.sync(); QCoreApplication::processEvents();
            require(first->isEnabled(), "disabled(false) did not restore interaction");
            root.disable = true; root.sync(); QCoreApplication::processEvents();
            root.setDisabled = false; root.first = 1; root.sync(); QCoreApplication::processEvents();
            require(root.button(0) == first && first->isEnabled() && root.writes == 0,
                "omitting disabled lost identity or retained disabled state");
            int external = 0;
            QObject::connect(second, &QRadioButton::clicked, [&]() { ++external; });
            second->click(); settle();
            require(static_cast<int>(root.first) == 2 && root.writes == 1 && root.clicks == 1 && external == 1
                && !first->isChecked() && second->isChecked(), "native radio edit failed or duplicated a commit");
            root.version = 2; root.other = true; root.sync();
            first->click(); settle();
            require(static_cast<int>(root.first) == 2 && static_cast<int>(root.second) == 1 && root.writes == 2 && root.clicks == 3, "radio rebind/callback reuse failed");
            root.second = 2; settle();
            require(second->isChecked() && root.writes == 2, "programmatic value caused feedback");
            for(int i = 0; i < 12; ++i) root.sync();
            second->click(); settle();
            require(root.writes == 3 && external == 2 && root.button(1) == second, "redraw duplicated callbacks or native widgets");
            // Preserve PUI's direct setChecked behavior with Qt auto-exclusivity.
            root.second = 99; settle();
            require(second->isChecked(), "unknown value diverged from Qt's auto-exclusive retained selection");
            root.action = []() { throw std::runtime_error("expected radio setter failure"); };
            first->click(); settle();
            require(root.writes == 4 && static_cast<int>(root.second) == 1, "native setter exception escaped dispatch");
            root.action = [&]() { root.show = false; root.sync(); };
            QPointer<QWidget> removed = first;
            second->click(); settle();
            require(removed.isNull() && root.writes == 5, "radio setter self-removal retained native widgets");
            root.action = {}; root.show = true; root.sync();
            auto declaration = static_cast<CPPUI::Qt::RadioButton<int> *>(root.children->front()->children->front()->children->front());
            delete root.button(0); require(!declaration->ui, "external radio deletion left stale access");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "radio teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

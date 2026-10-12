#include "cppui/qt/checkbox.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include <QApplication>
#include <QEvent>
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    class Root: public CPPUI::Qt::QtView {
        public:
        CPPUI::State<bool> first{false}, second{true};
        bool useSecond = false, show = true, fail = false;
        int writes = 0, clicks = 0;
        std::string text = "First";
        std::function<void()> action;
        std::weak_ptr<int> capture;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) if(show) {
                auto token = std::make_shared<int>(1);
                capture = token;
                auto state = useSecond ? &second : &first;
                CPPUI::Binding<bool> binding([this, state]() {
                    if(fail) throw std::runtime_error("expected getter failure");
                    return static_cast<bool>(*state);
                }, [this, state, token](bool value) {
                    ++writes;
                    auto invoke = action;
                    *state = value;
                    if(invoke) invoke();
                    require(*token == 1, "active setter lost its captures");
                });
                CPPUI::Checkbox(text, binding).click([this, state]() {
                    ++clicks;
                    require(static_cast<bool>(*state), "click ran before checkbox commit");
                });
                CPPUI::Checkbox("Mirror", *state);
            }
        }
        CPPUI::Node * column() { return children->front()->children->front(); }
        QCheckBox * widget(int index = 0) { return static_cast<QCheckBox *>(column()->children->at(index)->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    int result = 0;
    try {
        int external = 0, deleted = 0;
        {
            Root root;
            root.sync();
            auto object = root.widget();
            require(!object->isChecked(), "initial binding value was ignored");
            QObject::connect(object, &QCheckBox::toggled, [&external]() { ++external; });
            QObject::connect(object, &QObject::destroyed, [&deleted]() { ++deleted; });
            auto capture = root.capture;
            object->toggle();
            QCoreApplication::processEvents();
            require(static_cast<bool>(root.first) && root.writes == 1 && external == 1, "native edit committed more than once or lost a listener");
            require(root.widget() == object && root.widget(1)->isChecked() && capture.expired(), "redraw lost reuse, mirror state or callback cleanup");
            object->clicked(true);
            require(root.clicks == 1, "checkbox click was not dispatched");
            root.first = false;
            QCoreApplication::processEvents();
            require(!object->isChecked() && root.writes == 1, "programmatic update caused a feedback write");
            root.useSecond = true;
            root.text = "Second";
            root.sync();
            require(object->isChecked() && object->text().toStdString() == "Second", "rebind did not update checked state/text");
            object->toggle();
            QCoreApplication::processEvents();
            require(!static_cast<bool>(root.second) && !static_cast<bool>(root.first) && root.writes == 2, "rebind wrote the previous model");
            root.fail = true;
            bool threw = false;
            try { root.sync(); } catch(const std::runtime_error &) { threw = true; }
            require(threw && root.widget() == object, "getter failure discarded mounted UI");
            root.fail = false;
            root.sync();
            root.action = []() { throw std::runtime_error("expected setter failure"); };
            object->toggle();
            QCoreApplication::processEvents();
            require(root.writes == 3 && static_cast<bool>(root.second), "setter failure escaped native callback boundary");
            root.action = [&root]() { root.show = false; root.sync(); };
            capture = root.capture;
            object->toggle();
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            require(root.column()->children->empty() && capture.expired() && deleted == 1, "setter self-removal leaked native/callback data");
            root.action = {};
            root.show = true;
            for(int i = 0; i < 20; ++i) { root.sync(); root.first = !static_cast<bool>(root.first); }
            auto checkbox = static_cast<CPPUI::Qt::Checkbox *>(root.column()->children->front());
            delete root.widget();
            require(!checkbox->ui, "external native deletion left a stale checkbox");
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(QApplication::topLevelWidgets().isEmpty(), "checkbox teardown retained native content");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; result = 1; }
    return result;
}

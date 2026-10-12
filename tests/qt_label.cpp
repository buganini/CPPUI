#include "cppui/qt/label.hpp"
#include "cppui/qt/window.hpp"
#include <QEvent>
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void press(QWidget * widget) {
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(2, 2), QPointF(2, 2),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(widget, &event);
    }
    class Listener: public QObject {
        public:
        int calls = 0;
        bool eventFilter(QObject *, QEvent * event) override {
            if(event->type() == QEvent::MouseButtonPress) ++calls;
            return false;
        }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        bool show = true, handler = true, selectable = false;
        int value = 1, clicked = 0;
        std::weak_ptr<int> capture;
        std::function<void()> action;
        void content() override {
            for(auto window: CPPUI::Window()) if(show) {
                auto & label = CPPUI::Label("<b>literal</b>", selectable).id("label");
                if(handler) {
                    auto token = std::make_shared<int>(value);
                    capture = token;
                    label.click([this, token]() {
                        clicked = *token;
                        auto invoke = action;
                        if(invoke) invoke();
                        require(*token == clicked, "active label callback lost its capture");
                    });
                }
            }
        }
        CPPUI::Qt::Label * label() { return static_cast<CPPUI::Qt::Label *>(children->front()->children->front()); }
    };
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        {
            Root root;
            root.sync();
            auto widget = static_cast<QLabel *>(root.label()->ui);
            Listener listener;
            widget->installEventFilter(&listener);
            require(widget->textFormat() == Qt::PlainText && widget->cursor().shape() == Qt::PointingHandCursor, "clickable label format/cursor is wrong");
            press(widget);
            require(root.clicked == 1 && listener.calls == 1, "native press did not invoke label callback");
            auto capture = root.capture;
            root.value = 2;
            root.selectable = true;
            root.sync();
            require(root.label()->ui == widget && capture.expired(), "label reuse retained old callback or replaced widget");
            require(widget->textInteractionFlags() == Qt::TextSelectableByMouse, "click handler displaced selection support");
            press(widget);
            require(root.clicked == 2 && listener.calls == 2, "label reused stale callback or lost external filter");
            root.action = [&root]() { root.value = 3; root.sync(); };
            press(widget);
            require(root.clicked == 2, "active callback changed during redraw");
            root.action = {};
            press(widget);
            require(root.clicked == 3, "next press used retired callback");
            root.handler = false;
            root.selectable = false;
            capture = root.capture;
            root.sync();
            require(capture.expired() && widget->cursor().shape() == Qt::ArrowCursor && widget->textInteractionFlags() == Qt::NoTextInteraction,
                    "omitted label properties did not reset");
            press(widget);
            require(root.clicked == 3 && listener.calls == 5, "omitted callback remained active or listener was removed");
            root.handler = true;
            root.action = [&root]() { root.sync(); root.show = false; root.sync(); };
            root.sync();
            capture = root.capture;
            press(widget);
            require(root.children->front()->children->empty() && capture.expired(), "label self-removal retained declaration/capture");
            root.clicked = 0;
            press(widget);
            require(root.clicked == 0, "deferred-deletion widget dispatched a removed callback");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            root.show = true;
            root.sync();
            widget = static_cast<QLabel *>(root.label()->ui);
            root.action = [widget]() { delete widget; };
            press(widget);
            require(!root.label()->ui, "self-deleting native label did not invalidate observer");
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(QApplication::topLevelWidgets().isEmpty(), "label teardown retained native content");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

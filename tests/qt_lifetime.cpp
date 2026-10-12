#include "cppui/qt/application.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/state.hpp"
#include <stdexcept>
#include <QLineEdit>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<bool> show{true};
        int version = 1;
        int called = 0;
        bool removeOnClick = false;
        bool finished = false;
        bool focusWasClear = false;
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) {
                for(auto column: CPPUI::VBox()) {
                    if(show) {
                        const int currentVersion = version;
                        CPPUI::Button("click").click([this, currentVersion]() {
                            focusWasClear = QApplication::focusWidget() == nullptr;
                            called += currentVersion;
                            if(removeOnClick) show = false;
                            // Must survive destruction of the emitting declaration.
                            finished = true;
                        });
                    }
                    CPPUI::Label("tail");
                }
            }
        }
        QPushButton * button() {
            return static_cast<QWidget *>(children->front()->outer())->findChild<QPushButton *>();
        }
    };
    void borrowedApplication(int argc, char ** argv, QApplication & qt) {
        int externalCalls = 0;
        {
            Root root(argc, argv);
            root.redraw();
            auto button = root.button();
            QObject::connect(button, &QPushButton::clicked, &qt, [&]() { ++externalCalls; });
            root.version = 2;
            root.redraw();
            require(root.button() == button, "redraw replaced the native button");
            button->click();
            require(root.called == 2, "button used an old or duplicate callback");
            require(externalCalls == 1, "redraw disconnected application listeners");

            root.removeOnClick = true;
            root.redraw();
            root.finished = false;
            button->click();
            require(root.called == 4 && root.finished, "self-removal invalidated active dispatch");
            button->click();
            require(root.called == 4, "removed declaration received a deferred-native click");
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            for(int i = 0; i < 30; ++i) {
                root.show = true;
                root.show = false;
                QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            }
        }
        require(QCoreApplication::instance() == &qt, "borrowed QApplication was destroyed");
        require(qt.topLevelWidgets().empty(), "application teardown leaked native roots");

        {
            Root root(argc, argv);
            root.redraw();
            auto window = root.children->front();
            auto column = window->children->front();
            delete static_cast<QWidget *>(window->outer());
            require(!window->outer() && !column->outer(), "native parent deletion left stale pointers");
            for(auto child: *column->children) {
                require(!child->outer(), "parent deletion left a stale child pointer");
            }
        }
    }
    void focusBeforeClick(int argc, char ** argv, QApplication & qt) {
        Root root(argc, argv);
        root.redraw();
        auto window = static_cast<QWidget *>(root.children->front()->outer());
        QLineEdit editor(window);
        editor.show();
        window->activateWindow();
        editor.setFocus();
        qt.processEvents();
        require(QApplication::focusWidget() == &editor, "test editor did not receive focus");
        int focusChanges = 0;
        auto connection = QObject::connect(&qt, &QApplication::focusChanged, &qt,
            [&](QWidget * before, QWidget * after) {
                if(before == &editor && after == nullptr) {
                    ++focusChanges;
                    root.version = 7;
                    root.redraw();
                }
            });
        root.button()->click();
        QObject::disconnect(connection);
        require(focusChanges == 1 && root.focusWasClear, "button callback ran before focus was cleared");
        require(root.called == 1 && root.finished, "focus-out redraw invalidated the active button callback");
        root.button()->click();
        require(root.called == 8, "next click did not use the callback installed by focus-out redraw");
    }
}
int main(int argc, char ** argv) {
    try {
        if(argc > 1 && std::string(argv[1]) == "--owned") {
            {
                Root root(argc, argv);
                root.redraw();
                require(qobject_cast<QApplication *>(QCoreApplication::instance()), "application was not created");
            }
            require(!QCoreApplication::instance(), "owned QApplication leaked");
        } else {
            QApplication qt(argc, argv);
            borrowedApplication(argc, argv, qt);
            focusBeforeClick(argc, argv, qt);
        }
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

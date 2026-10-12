#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include <QTimer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    class Root: public CPPUI::Application {
        public:
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) CPPUI::Label("quit test");
        }
    };
    class Watchdog: public QTimer {
        public:
        bool expired = false;
        Watchdog() {
            setSingleShot(true);
            QObject::connect(this, &QTimer::timeout, this, [this]() {
                expired = true;
                QCoreApplication::exit(99);
            });
            start(2000);
        }
    };
    void queuedQuit(int argc, char ** argv, QApplication & qt) {
        Root root(argc, argv);
        root.sync();
        bool callbackReturned = false;
        bool quitAfterCallback = false;
        auto connection = QObject::connect(&qt, &QCoreApplication::aboutToQuit, &qt, [&]() {
            quitAfterCallback = callbackReturned;
        });
        QTimer::singleShot(0, &qt, [&]() {
            root.quit();
            root.quit();
            callbackReturned = true;
        });
        Watchdog watchdog;
        root.start();
        QObject::disconnect(connection);
        require(!watchdog.expired && quitAfterCallback, "quit did not leave the event loop after dispatch returned");
    }
    void cancelledQuit(int argc, char ** argv, QApplication & qt) {
        Root root(argc, argv);
        root.sync();
        bool nextTurnRan = false;
        QTimer::singleShot(0, &qt, [&]() {
            {
                Root removed(argc, argv);
                removed.sync();
                removed.quit();
            }
            QTimer::singleShot(25, &qt, [&]() {
                nextTurnRan = true;
                root.quit();
            });
        });
        Watchdog watchdog;
        root.start();
        require(!watchdog.expired && nextTurnRan, "a destroyed application's queued quit affected the surviving application");
    }
}

int main(int argc, char ** argv) {
    try {
        if(argc > 1 && std::string(argv[1]) == "--owned") {
            {
                Root root(argc, argv);
                root.quit(); // Safe even before lazy QApplication creation.
                root.sync();
                Watchdog watchdog;
                root.start();
                require(!watchdog.expired, "quit queued before startup was lost");
            }
            require(!QCoreApplication::instance(), "owned QApplication leaked after quit");
        } else {
            QApplication qt(argc, argv);
            queuedQuit(argc, argv, qt);
            cancelledQuit(argc, argv, qt);
            require(QCoreApplication::instance() == &qt, "quit destroyed the borrowed QApplication");
        }
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

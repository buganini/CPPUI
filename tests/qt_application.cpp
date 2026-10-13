#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include <QTimer>
#include <QImage>
#include <QTemporaryDir>
#include <QStyleFactory>
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
    void initialIcon(int argc, char ** argv, QApplication & qt, const std::string & path) {
        QPixmap blue(16, 16); blue.fill(::Qt::blue);
        qt.setWindowIcon(QIcon(blue));
        const auto original = qt.windowIcon().cacheKey();
        {
            Root omitted(argc, argv); omitted.sync();
            require(qt.windowIcon().cacheKey() == original, "omitted icon replaced the borrowed application's icon");
        }
        {
            Root configured(argc, argv, std::string(path)); configured.sync();
            require(qt.windowIcon().pixmap(16, 16).toImage().pixelColor(8, 8) == QColor(::Qt::red), "explicit application icon was not loaded");
            qt.setWindowIcon(QIcon(blue));
            const auto replacement = qt.windowIcon().cacheKey();
            configured.sync();
            require(qt.windowIcon().cacheKey() == replacement, "redraw reapplied an initial-only application icon");
        }
        require(QCoreApplication::instance() == &qt && !qt.windowIcon().isNull(), "icon application changed borrowed native ownership");
    }
    void destroyedQueuedQuit(int argc, char ** argv, QApplication & qt) {
        Root survivor(argc, argv); survivor.sync();
        Root removed(argc, argv); removed.sync();
        // Keep QApplication running while the other root loses its content.
        const bool oldQuitOnClose = qt.quitOnLastWindowClosed();
        qt.setQuitOnLastWindowClosed(false);
        bool nextTurnRan = false;
        QTimer::singleShot(0, &qt, [&]() {
            removed.quit();
            removed.clearChildren();
            removed.quit(); // Already destroyed: must not enqueue another quit.
            QTimer::singleShot(25, &qt, [&]() { nextTurnRan = true; survivor.quit(); });
        });
        Watchdog watchdog; survivor.start();
        qt.setQuitOnLastWindowClosed(oldQuitOnClose);
        require(!watchdog.expired && nextTurnRan, "queued quit ignored logical destruction before dispatch");
    }
    void applicationOptions(int argc, char ** argv, QApplication & qt) {
        const auto originalSheet = qt.styleSheet();
        qt.setStyleSheet("color:#123456;");
        {
            Root root(argc, argv); root.sync();
            require(qt.styleSheet() == "color:#123456;", "omitted application styling changed a borrowed stylesheet");
            CPPUI::QtOptions clear;
            clear.StyleSheet = CPPUI::QtStyleSheet{};
            root.qt(clear); root.sync();
            require(qt.styleSheet().isEmpty(), "explicit empty application stylesheet did not clear borrowed styling");
        }
        {
            Root root(argc, argv);
            CPPUI::QtOptions options;
            auto style = QStyleFactory::create("Fusion");
            bool styleDestroyed = false;
            QObject::connect(style, &QObject::destroyed, &qt, [&]() { styleDestroyed = true; });
            options.Style = style; options.StyleSheet = CPPUI::QtStyleSheet{{"color", "#123456"}};
            root.style(0xff0000, {}, 18).qt(options);
            (*options.StyleSheet)["color"] = "#ffffff";
            root.sync();
            require(qt.styleSheet().contains("color:#123456;") && qt.styleSheet().contains("font:18pt;"),
                "application options were not copied or did not override generic styles");
            root.sync();
            require(!styleDestroyed, "repeated application update destroyed its adopted style");
            qt.setStyle(QStyleFactory::create("Fusion"));
            require(styleDestroyed, "QApplication did not own the requested style");
        }
        qt.setStyleSheet(originalSheet);
    }
}

int main(int argc, char ** argv) {
    try {
        QTemporaryDir directory;
        require(directory.isValid(), "could not create icon fixture directory");
        const auto iconPath = directory.filePath("icon.png");
        QImage icon(16, 16, QImage::Format_ARGB32); icon.fill(::Qt::red);
        require(icon.save(iconPath), "could not save application icon fixture");
        if(argc > 1 && std::string(argv[1]) == "--owned") {
            {
                Root root(argc, argv, iconPath.toStdString());
                root.quit(); // Safe even before lazy QApplication creation.
                root.sync();
                require(QApplication::windowIcon().pixmap(16, 16).toImage().pixelColor(8, 8) == QColor(::Qt::red), "owned application's icon was not initialized");
                Watchdog watchdog;
                root.start();
                require(!watchdog.expired, "quit queued before startup was lost");
            }
            require(!QCoreApplication::instance(), "owned QApplication leaked after quit");
        } else {
            QApplication qt(argc, argv);
            initialIcon(argc, argv, qt, iconPath.toStdString());
            applicationOptions(argc, argv, qt);
            queuedQuit(argc, argv, qt);
            cancelledQuit(argc, argv, qt);
            destroyedQueuedQuit(argc, argv, qt);
            require(QCoreApplication::instance() == &qt, "quit destroyed the borrowed QApplication");
        }
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

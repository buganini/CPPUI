#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTemporaryDir>
#include <QPixmap>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    class Root: public CPPUI::Application {
        public:
        bool title = true, size = true, icon = true, maximized = false, fullscreen = false;
        std::string caption = "First", path, text = "content";
        int width = 320;
        using Application::Application;
        void content() override {
            auto & window = CPPUI::Window().maximize(maximized).fullscreen(fullscreen);
            if(title) window.title(caption);
            if(size) window.size(width, 240);
            if(icon) window.icon(path);
            for(auto scope: window) for(auto column: CPPUI::VBox()) CPPUI::Label(text);
        }
        CPPUI::Qt::Window * declaration() { return static_cast<CPPUI::Qt::Window *>(children->front()); }
        QMainWindow * native() { return static_cast<QMainWindow *>(declaration()->ui); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        QTemporaryDir directory;
        const auto path = directory.filePath("icon.png");
        QPixmap icon(16, 16); icon.fill(::Qt::red);
        require(icon.save(path), "could not create test icon");
        {
            Root root(argc, argv); root.path = path.toStdString(); root.sync();
            auto window = root.native();
            require(window->windowTitle() == "First" && window->size() == QSize(320, 240)
                && !window->windowIcon().isNull(), "window properties were not applied");
            const auto iconKey = window->windowIcon().cacheKey();
            window->resize(410, 300);
            root.icon = false; root.sync();
            require(root.native() == window && window->size() == QSize(410, 300)
                && window->windowIcon().cacheKey() == iconKey, "redraw reset user size or omitted icon");
            root.width = 350; root.caption = "第二個"; root.sync();
            require(window->size() == QSize(350, 240) && window->windowTitle() == QString::fromUtf8("第二個"), "changed size/title did not apply");
            root.size = false; root.title = false; root.sync();
            window->resize(420, 310);
            root.size = true; root.sync();
            require(window->size() == QSize(350, 240) && window->windowTitle() == QString::fromUtf8("第二個"), "size redeclaration or title omission diverged from PUI");
            root.declaration()->maximize();
            require(window->isMaximized(), "imperative maximize failed");
            root.sync();
            require(window->isMaximized(), "redraw reapplied initial maximize flag");
            root.declaration()->maximize(false).fullscreen();
            require(window->isFullScreen(), "imperative fullscreen failed");
            root.sync();
            require(window->isFullScreen(), "redraw reapplied initial fullscreen flag");
            root.declaration()->fullscreen(false);
            QLineEdit editor(window);
            editor.show(); window->activateWindow(); editor.setFocus();
            QCoreApplication::processEvents();
            require(QApplication::focusWidget() == &editor, "test editor did not receive focus");
            QKeyEvent typed(QEvent::KeyPress, ::Qt::Key_A, ::Qt::NoModifier, "a");
            QCoreApplication::sendEvent(&editor, &typed);
            int committed = 0;
            QObject::connect(&editor, &QLineEdit::editingFinished, [&]() { ++committed; });
            QMouseEvent event(QEvent::MouseButtonPress, QPointF(200, 150), QPointF(200, 150),
                ::Qt::LeftButton, ::Qt::LeftButton, ::Qt::NoModifier);
            QCoreApplication::sendEvent(window, &event);
            require(QApplication::focusWidget() != &editor && committed == 1, "window press did not commit focused editor");
        }
        {
            Root root(argc, argv); root.sync();
            QCoreApplication::processEvents();
            auto window = root.native();
            require(window->size() == QSize(320, 240), "initial content changed declared window size");
            root.text = std::string(1000, 'x'); root.sync();
            QCoreApplication::processEvents(); QCoreApplication::processEvents();
            require(window->centralWidget()->minimumSizeHint().width() > 320
                && window->size() == QSize(320, 240), "growing content forced the top-level window to resize");
            root.text = "short"; root.sync();
            QCoreApplication::processEvents();
            require(root.native() == window && window->size() == QSize(320, 240), "shrinking content changed window size or identity");
            window->resize(460, 300);
            root.text = std::string(1000, 'x'); root.sync();
            QCoreApplication::processEvents(); QCoreApplication::processEvents();
            require(window->size() == QSize(460, 300), "growing content overwrote the user's resized window");
        }
        {
            Root root(argc, argv); root.maximized = true; root.sync();
            require(root.native()->isMaximized(), "initial maximized flag was lost");
            root.declaration()->maximize(false); root.sync();
            require(!root.native()->isMaximized(), "initial maximize repeated on redraw");
            auto declaration = root.declaration();
            delete root.native();
            require(!declaration->ui, "native destruction left a stale window");
        }
        {
            Root root(argc, argv); root.fullscreen = true; root.sync();
            require(root.native()->isFullScreen(), "initial fullscreen flag was lost");
        }
        require(qt.topLevelWidgets().isEmpty(), "window teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

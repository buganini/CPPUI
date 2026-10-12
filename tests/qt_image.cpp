#include "cppui/qt/image.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include <QTemporaryDir>
#include <QFile>
#include <QMouseEvent>
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    class Root: public CPPUI::Application {
        public:
        std::string path;
        int clicked = 0;
        bool show = true, clickable = true;
        std::function<void()> action;
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) if(show) {
                auto & image = CPPUI::Image(path);
                if(clickable) image.click([this]() { ++clicked; auto invoke = action; if(invoke) invoke(); });
            }
        }
        QLabel * native() { return static_cast<QLabel *>(children->front()->children->front()->outer()); }
    };
    void click(QWidget * widget) {
        QMouseEvent event(QEvent::MouseButtonPress, QPointF(2, 2), QPointF(2, 2), ::Qt::LeftButton, ::Qt::LeftButton, ::Qt::NoModifier);
        QCoreApplication::sendEvent(widget, &event);
    }
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        QTemporaryDir directory;
        auto first = directory.filePath("first.png"), second = directory.filePath("second.png");
        QPixmap source(20, 10); source.fill(::Qt::red); require(source.save(first), "test image save failed");
        source = QPixmap(10, 30); source.fill(::Qt::blue); require(source.save(second), "second test image save failed");
        {
            Root root(argc, argv); root.path = first.toStdString(); root.sync();
            auto label = root.native();
            require(label->pixmap().size() == QSize(20, 10) && label->cursor().shape() == ::Qt::PointingHandCursor, "image size/click support missing");
            const auto cache = label->pixmap().cacheKey();
            for(int i = 0; i < 12; ++i) root.sync();
            require(root.native() == label && label->pixmap().cacheKey() == cache, "unchanged image was reloaded or remounted");
            click(label); require(root.clicked == 1, "image click callback missing");
            root.path = second.toStdString(); root.clickable = false; root.sync();
            require(root.native() == label && label->pixmap().size() == QSize(10, 30)
                && label->cursor().shape() != ::Qt::PointingHandCursor, "changed source/callback omission was ignored");
            click(label); require(root.clicked == 1, "omitted image callback was retained");
            source.fill(::Qt::green); require(source.save(second), "rewritten test image save failed");
            QFile file(second); require(file.open(QIODevice::ReadOnly), "mtime test could not open image");
            require(file.setFileTime(QDateTime::currentDateTime().addSecs(2), QFileDevice::FileModificationTime), "mtime test could not change timestamp");
            file.close(); root.sync();
            require(label->pixmap().toImage().pixelColor(0, 0) == QColor(::Qt::green), "same-path mtime update used stale pixels");
            root.path = directory.filePath("missing.png").toStdString();
            bool failed = false; try { root.sync(); } catch(const std::runtime_error &) { failed = true; }
            require(failed && root.native() == label && !label->pixmap().isNull(), "missing file discarded previous image");
            root.path = first.toStdString(); root.clickable = true;
            root.action = [&]() { root.show = false; root.sync(); }; root.sync();
            QPointer<QWidget> alive = label; click(label);
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            require(root.clicked == 2 && alive.isNull(), "image self-removal retained native/callback storage");
            root.show = true; root.action = {}; root.sync();
            auto declaration = static_cast<CPPUI::Qt::Image *>(root.children->front()->children->front());
            delete root.native(); require(!declaration->ui, "native image deletion left a stale observer");
        }
        require(qt.topLevelWidgets().isEmpty(), "image teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

#include "cppui/qt/scroll.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        for(int i = 0; i < 4; ++i) {
            QCoreApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    class Content: public CPPUI::Qt::Label {
        int width, height;
        public:
        Content(int width, int height): Label("large content"), width(width), height(height) {}
        void update(CPPUI::Qt::BaseWidget * prev) override { Label::update(prev); ui->setFixedSize(width, height); }
    };
    class Root: public CPPUI::Application {
        public:
        int width = 1200, height = 800;
        bool show = true, prefix = false, end = true, horizontal = true;
        std::string contentId = "content";
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window().size(320, 240)) if(show) {
                auto & scroller = CPPUI::Scroll({}, horizontal);
                if(end) scroller.scrollX(CPPUI::ScrollEnd).scrollY(CPPUI::ScrollEnd);
                for(auto scroll: scroller) {
                    if(prefix) CPPUI::Label("inserted").id("prefix");
                    (new Content(width, height))->id(contentId);
                    CPPUI::Label("ignored").id("ignored");
                }
            }
        }
        CPPUI::Qt::Scroll * declaration() { return static_cast<CPPUI::Qt::Scroll *>(children->front()->children->front()); }
        CPPUI::Qt::ScrollWidget * native() { return static_cast<CPPUI::Qt::ScrollWidget *>(declaration()->ui); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        require(std::signbit(CPPUI::ScrollEnd) && !std::signbit(0.0), "end anchor lost negative zero");
        {
            Root root(argc, argv); root.sync(); settle();
            auto scroll = root.native();
            auto content = scroll->widget();
            auto x = scroll->horizontalScrollBar(), y = scroll->verticalScrollBar();
            require(x->maximum() > 500 && y->maximum() > 300 && x->value() == x->maximum() && y->value() == y->maximum(), "initial end anchoring failed");
            require(scroll->verticalScrollBarPolicy() == ::Qt::ScrollBarAsNeeded && scroll->horizontalScrollBarPolicy() == ::Qt::ScrollBarAlwaysOn, "scroll policies diverged from PUI");
            root.width += 100; root.height += 100; root.sync(); settle();
            require(root.native() == scroll && scroll->widget() == content && x->value() == x->maximum() && y->value() == y->maximum(), "growth lost native identity or end anchoring");
            x->setValue(0); y->setValue(0); x->setValue(23); y->setValue(37);
            root.end = false; root.horizontal = false; root.width += 100; root.height += 100; root.sync(); settle();
            require(x->value() == 23 && y->value() == 37 && scroll->x.offset == 23 && scroll->y.offset == 37, "pre-sync offsets were not retained");
            require(scroll->horizontalScrollBarPolicy() == ::Qt::ScrollBarAlwaysOn, "redraw changed initial-only scrollbar policy");
            x->setValue(x->maximum()); y->setValue(y->maximum());
            x->setValue(x->maximum() - 25); y->setValue(y->maximum() - 30);
            root.width += 100; root.height += 100; root.sync(); settle();
            require(x->maximum() - x->value() == 25 && y->maximum() - y->value() == 30, "end-distance offsets changed on content resize");
            const int vertical = y->value();
            root.declaration()->scrollX(19);
            x->setRange(0, 2000);
            require(x->value() == 19 && y->value() == vertical, "horizontal range callback changed the wrong scrollbar");
            root.sync(); settle();
            QPointer<QWidget> oldContent = content;
            root.prefix = true; root.sync(); settle();
            require(oldContent && oldContent->isHidden() && scroll->widget() != oldContent, "first-child insertion deleted a retained sibling");
            root.prefix = false; root.sync(); settle();
            require(!scroll->widget() && oldContent && oldContent->isHidden(), "removal implicitly promoted an ignored child");
            root.contentId = "replacement"; root.sync(); settle();
            require(scroll->widget() && scroll->widget()->isVisible() && !oldContent, "first-child replacement failed or retained old content");
            for(int i = 0; i < 12; ++i) {
                root.show = false; root.sync(); settle();
                root.show = true; root.sync(); settle();
            }
            auto declaration = root.declaration();
            delete root.native(); require(!declaration->ui, "external scroll deletion left stale access");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "scroll teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

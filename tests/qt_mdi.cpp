#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/mdi.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    CPPUI::Node * find(CPPUI::Node & root, const char * id) {
        if(root.has_id && root._id == id) return &root;
        for(auto child: *root.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    class Group: public CPPUI::Qt::QtView {
        public:
        void content() override {
            for(auto sub: CPPUI::MdiSubWindow().id("nested")) CPPUI::Label("nested content");
        }
    };
    class Root: public CPPUI::Application {
        public:
        bool reverse = false, show = true, extra = true, grouped = false;
        using Application::Application;
        void first() {
            for(auto sub: CPPUI::MdiSubWindow().id("first")) {
                if(grouped) for(auto column: CPPUI::VBox().id("group")) CPPUI::Label("grouped");
                else CPPUI::Label("first").id("content");
                if(extra) CPPUI::Label("ignored").id("ignored");
            }
        }
        void content() override {
            for(auto window: CPPUI::Window().size(600, 400)) for(auto area: CPPUI::MdiArea().id("area")) if(show) {
                if(!reverse) first();
                (new Group())->id("view");
                CPPUI::Label("automatic wrapper").id("automatic");
                if(reverse) first();
            }
        }
        QWidget * widget(const char * id) { return static_cast<QWidget *>(find(*this, id)->outer()); }
        QMdiArea * area() { return static_cast<QMdiArea *>(widget("area")); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync(); settle();
            auto area = root.area();
            auto sub = static_cast<QMdiSubWindow *>(root.widget("first"));
            auto content = root.widget("content");
            auto automatic = root.widget("automatic");
            require(area->subWindowList().size() == 3 && sub->widget() == content, "MDI children/virtual view were not mounted");
            require(sub->isVisible() && content->isVisible(), "MDI content did not become visible");
            require(root.widget("ignored")->parentWidget() == sub && root.widget("ignored")->isHidden(), "extra frame child became visible or lost ownership");
            sub->resize(240, 170); sub->move(30, 40);
            const auto geometry = sub->geometry();
            for(int i = 0; i < 12; ++i) { root.sync(); settle(); }
            require(root.widget("first") == sub && sub->geometry() == geometry && root.widget("automatic") == automatic, "unchanged MDI redraw lost native geometry/identity");
            root.reverse = true; root.sync(); settle();
            require(root.widget("first") == sub && root.widget("content") == content && area->subWindowList().size() == 3, "keyed move leaked wrappers or remounted content");
            QPointer<QWidget> replaced = content;
            root.grouped = true; root.extra = false; root.sync(); settle();
            require(replaced.isNull() && sub->widget() == root.widget("group"), "single frame content replacement retained the old native widget");
            sub->close(); root.sync(); settle();
            require(sub->isHidden() && root.widget("first") == sub, "redraw reopened a natively closed subwindow");
            for(int i = 0; i < 10; ++i) {
                root.show = false; root.sync(); settle();
                require(area->subWindowList().isEmpty() && area->findChildren<QMdiSubWindow *>().isEmpty(), "removal retained automatic/explicit MDI wrappers");
                root.show = true; root.sync(); settle();
            }
            auto declaration = static_cast<CPPUI::Qt::MdiArea *>(find(root, "area"));
            auto child = static_cast<CPPUI::Qt::MdiSubWindow *>(find(root, "first"));
            delete area;
            require(!declaration->ui && !child->ui, "MDI native parent deletion left stale observers");
        }
        settle();
        require(qt.topLevelWidgets().isEmpty(), "MDI teardown leaked native widgets");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

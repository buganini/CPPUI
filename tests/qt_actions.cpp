#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/button.hpp"
#include <QPointer>
#include <QTemporaryDir>
#include <QPixmap>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    CPPUI::Node * find(CPPUI::Node & root, const char * id) {
        if(root.has_id && root._id == id) return &root;
        for(auto child: *root.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    class Root: public CPPUI::Application {
        public:
        bool show = true, showWidget = true, reverse = false, callback = true;
        int version = 1, called = 0;
        std::string icon;
        std::function<void()> action;
        std::weak_ptr<int> capture;
        using Application::Application;
        void item() {
            auto & item = CPPUI::MenuAction("Action " + std::to_string(version)); item.id("action");
            if(callback) {
                auto token = std::make_shared<int>(version); capture = token;
                item.trigger([this, token]() {
                    const auto before = *token;
                    called += before;
                    auto invoke = action; if(invoke) invoke();
                    require(*token == before, "active action lost its callback capture");
                });
            }
        }
        void content() override {
            for(auto window: CPPUI::Window().size(500, 300)) {
                if(show) {
                    for(auto bar: CPPUI::MenuBar().id("bar")) {
                        for(auto menu: CPPUI::Menu("Menu " + std::to_string(version)).id("menu")) {
                            if(reverse) item();
                            for(auto submenu: CPPUI::Menu("Nested").id("submenu")) CPPUI::MenuAction("nested action");
                            if(!reverse) item();
                        }
                    }
                    for(auto toolbar: CPPUI::ToolBar().id("toolbar")) {
                        if(reverse && showWidget) CPPUI::Button("widget").id("widget");
                        CPPUI::ToolBarAction("Tool", icon).trigger([this]() { ++called; }).id("tool");
                        if(!reverse && showWidget) CPPUI::Button("widget").id("widget");
                    }
                }
                for(auto central: CPPUI::VBox().id("central")) CPPUI::Label("central");
            }
        }
        QWidget * widget(const char * id) { return static_cast<QWidget *>(find(*this, id)->outer()); }
        QAction * nativeAction(const char * id = "action") { return static_cast<QAction *>(find(*this, id)->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        QTemporaryDir directory;
        auto path = directory.filePath("tool.png"); QPixmap image(16, 16); image.fill(::Qt::red); image.save(path);
        {
            Root root(argc, argv); root.icon = path.toStdString(); root.sync(); settle();
            auto window = static_cast<QMainWindow *>(root.children->front()->outer());
            auto central = root.widget("central");
            auto bar = static_cast<QMenuBar *>(root.widget("bar"));
            auto menu = static_cast<QMenu *>(root.widget("menu"));
            auto toolbar = static_cast<QToolBar *>(root.widget("toolbar"));
            auto widget = root.widget("widget");
            auto action = root.nativeAction();
            require(window->centralWidget() == central && window->menuWidget() == bar && toolbar->actions().size() == 2,
                "menu/toolbar routing replaced central content");
            require(menu->actions().size() == 2 && !root.nativeAction("tool")->icon().isNull(), "nested actions or toolbar icon missing");
            auto wrapper = qobject_cast<QWidgetAction *>(toolbar->actions().at(1));
            require(wrapper && wrapper->defaultWidget() == widget, "toolbar widget action missing");
            int external = 0;
            QObject::connect(action, &QAction::triggered, [&]() { ++external; });
            auto retiredCapture = root.capture;
            root.version = 2; root.sync(); settle();
            action->trigger();
            require(root.called == 2 && external == 1 && action->text() == "Action 2" && retiredCapture.expired(), "action reuse retained stale data or removed outside listeners");
            for(int i = 0; i < 12; ++i) {
                root.reverse = !root.reverse; root.sync(); settle();
                require(root.nativeAction() == action && root.widget("widget") == widget && toolbar->actions().size() == 2
                    && toolbar->findChildren<QWidgetAction *>().size() == 1, "keyed action/widget moves leaked wrappers or lost identity");
                require(menu->actions().at(root.reverse ? 0 : 1) == action, "nested action order is wrong");
            }
            QPointer<QWidgetAction> wrapperLife = wrapper;
            QPointer<QWidget> widgetLife = widget;
            root.showWidget = false; root.sync(); settle();
            require(widgetLife.isNull() && wrapperLife.isNull(), "removed toolbar widget retained its owning action");
            root.callback = false; root.sync(); action->trigger();
            require(root.called == 2 && external == 2, "omitted trigger callback was not cleared");
            root.callback = true; root.action = [&]() { root.show = false; root.sync(); }; root.sync();
            retiredCapture = root.capture;
            QPointer<QAction> actionLife = action;
            QPointer<QWidget> barLife = bar, toolbarLife = toolbar;
            action->trigger(); settle();
            require(root.called == 4 && retiredCapture.expired() && actionLife.isNull() && barLife.isNull() && toolbarLife.isNull()
                && window->centralWidget() == central, "trigger self-removal leaked resources or damaged central content");
            root.action = {}; root.showWidget = true;
            for(int i = 0; i < 12; ++i) {
                root.show = true; root.sync(); settle();
                root.show = false; root.sync(); settle();
            }
            root.show = true; root.sync(); settle();
            auto declaration = static_cast<CPPUI::Qt::MenuAction *>(find(root, "action"));
            delete window;
            require(!declaration->ui, "native menu parent deletion left a dangling QAction observer");
        }
        settle();
        require(qt.topLevelWidgets().isEmpty(), "menu/toolbar teardown retained native objects");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

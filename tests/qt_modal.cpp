#include "cppui/qt/application.hpp"
#include "cppui/qt/modal.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/splitter.hpp"
#include "cppui/qt/tab.hpp"
#include <QPointer>
#include <sstream>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    CPPUI::Node * find(CPPUI::Node & root, const std::string & id) {
        if(root.has_id && root._id == id) return &root;
        for(auto child: *root.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<int> & first;
        CPPUI::State<int> & second;
        bool useSecond = false, include = true, fail = false, declareSize = true, declareTitle = true;
        bool initialMaximize = false, initialFullscreen = false;
        int width = 300, generation = 0, opened = 0, closed = 0, lastGeneration = -1;
        std::shared_ptr<int> token = std::make_shared<int>(1);
        std::function<void()> afterOpen, afterClose;
        Root(int argc, char ** argv, CPPUI::State<int> & a, CPPUI::State<int> & b)
            : Application(argc, argv), first(a), second(b) {}
        void content() override {
            if(fail) throw std::runtime_error("modal builder failure");
            for(auto window: CPPUI::Window().id("window")) {
                for(auto box: CPPUI::VBox().id("box")) {
                    CPPUI::Label("before").id("before");
                    if(include) {
                        const auto current = generation;
                        auto keep = token;
                        auto & modal = CPPUI::Modal(useSecond ? second : first)
                            .open([this, current, keep]() { ++opened; lastGeneration = current; if(afterOpen) afterOpen(); })
                            .close([this, current, keep]() { ++closed; lastGeneration = current; if(afterClose) afterClose(); })
                            .maximize(initialMaximize).fullscreen(initialFullscreen);
                        if(declareSize) modal.size(width, 180);
                        if(declareTitle) modal.title("Modal title");
                        for(auto scope: modal.id("modal")) {
                            CPPUI::Button("close").click([this]() { (useSecond ? second : first) = 0; });
                        }
                    }
                    CPPUI::Label("after").id("after");
                }
            }
        }
        CPPUI::Qt::Modal * modal() { return dynamic_cast<CPPUI::Qt::Modal *>(find(*this, "modal")); }
        CPPUI::Qt::ModalWidget * native() { return static_cast<CPPUI::Qt::ModalWidget *>(modal()->ui); }
    };
    class Routing: public CPPUI::Application {
        CPPUI::State<bool> & status;
        public:
        Routing(int argc, char ** argv, CPPUI::State<bool> & status): Application(argc, argv), status(status) {}
        void content() override {
            CPPUI::Modal(status).id("top");
            for(auto window: CPPUI::Window().id("window")) {
                CPPUI::Modal(status).id("window-modal");
                for(auto splitter: CPPUI::Splitter().id("splitter")) {
                    CPPUI::Label("first");
                    CPPUI::Modal(status).id("splitter-modal");
                    for(auto tabs: CPPUI::Tabs().id("tabs")) {
                        for(auto tab: CPPUI::Tab("page")) {
                            CPPUI::Label("page content");
                            for(auto modal: CPPUI::Modal(status).id("tab-modal")) {
                                CPPUI::Modal(status).id("nested-modal");
                                CPPUI::Label("modal content");
                            }
                        }
                    }
                }
            }
        }
    };
    struct Visibility {
        int value;
        explicit operator bool() const { return value > 0; }
    };
    class External: public CPPUI::Application {
        CPPUI::State<int> & status;
        public:
        External(int argc, char ** argv, CPPUI::State<int> & status): Application(argc, argv), status(status) {}
        void content() override {
            CPPUI::Binding<Visibility> binding([this]() { return Visibility{static_cast<int>(status)}; },
                [this](const Visibility & value) { status = value.value; });
            CPPUI::Modal(binding, Visibility{-7}).id("modal");
        }
    };
    class Nested: public CPPUI::Qt::QtView {
        CPPUI::State<bool> status{true};
        int & callbacks;
        public:
        explicit Nested(int & callbacks): callbacks(callbacks) {}
        void content() override {
            CPPUI::Modal(status).open([this]() { ++callbacks; status = false; })
                .close([this]() { ++callbacks; }).id("nested-modal");
        }
    };
    class NestedRoot: public CPPUI::Application {
        int & callbacks;
        public:
        bool include = true;
        NestedRoot(int argc, char ** argv, int & callbacks): Application(argc, argv), callbacks(callbacks) {}
        void content() override {
            for(auto window: CPPUI::Window()) {
                for(auto box: CPPUI::VBox()) {
                    if(include) new Nested(callbacks);
                }
            }
        }
    };
}

int main(int argc, char ** argv) {
    QApplication application(argc, argv);
    application.setQuitOnLastWindowClosed(false);
    try {
        CPPUI::State<int> first{0}, second{0};
        {
            Root root(argc, argv, first, second);
            root.redraw();
            auto native = root.native();
            QPointer<QWidget> nativeLife = native;
            auto box = static_cast<CPPUI::Qt::BaseLayout *>(find(root, "box"));
            require(native->isModal() && !native->isVisible() && box->layout->count() == 2,
                    "closed Modal consumed a box slot or became visible");
            require(native->parentWidget() == box->ui && native->layout()->count() == 1,
                    "Modal has no native owner or lost its content");
            first = 1;
            require(root.opened == 0, "open callback ran during reconciliation");
            settle();
            require(root.opened == 1 && native->isVisible() && native->windowTitle() == "Modal title",
                    "bound status did not open the modal");
            root.generation = 1;
            auto retiredPayload = std::weak_ptr<int>(root.token);
            root.token.reset();
            first = 2;
            settle();
            require(root.native() == native && root.opened == 1, "truthy redraw reopened or replaced Modal");
            require(retiredPayload.expired(), "retired modal callbacks retained their captures");
            native->resize(410, 220);
            root.redraw();
            require(native->size() == QSize(410, 220), "unchanged declared size reset the user's resize");
            root.width = 320;
            root.redraw();
            require(native->size() == QSize(320, 180), "changed declared size did not apply");
            root.declareSize = false;
            root.declareTitle = false;
            root.redraw();
            native->resize(420, 230);
            root.declareSize = true;
            root.redraw();
            require(native->size() == QSize(320, 180) && native->windowTitle() == "Modal title",
                    "size redeclaration or omitted title diverged from PUI");
            native->reject();
            settle();
            require(static_cast<int>(first) == 0 && !native->isVisible()
                    && root.closed == 1 && root.lastGeneration == 1,
                    "native rejection did not use the current binding/callback");
            root.useSecond = true;
            root.redraw();
            first = 1;
            require(!native->isVisible(), "old binding remained subscribed after rebinding");
            second = 1;
            settle();
            native->close();
            settle();
            require(static_cast<int>(first) == 1 && static_cast<int>(second) == 0 && root.closed == 2,
                    "native window close wrote the retired binding");

            root.afterOpen = [&]() { second = 2; };
            second = 1;
            settle();
            require(static_cast<int>(second) == 2 && root.opened == 3 && root.native() == native,
                    "open callback could not safely redraw the modal");
            root.afterOpen = {};
            root.afterClose = [&]() { root.include = false; root.redraw(); };
            native->accept();
            settle();
            require(!root.modal() && nativeLife.isNull() && root.closed == 3,
                    "close callback self-removal retained native resources");
            root.afterClose = {};

            root.include = true;
            second = 0;
            root.redraw();
            second = 1;
            settle();
            nativeLife = root.native();
            root.include = false;
            root.redraw();
            // A queued native event cannot write a retired binding.
            static_cast<QDialog *>(nativeLife.data())->reject();
            settle();
            require(root.closed == 4 && static_cast<int>(second) == 1 && nativeLife.isNull(),
                    "removal lost its close notification or wrote through a detached binding");

            root.include = true;
            root.initialMaximize = true;
            root.redraw();
            settle();
            require(root.native()->isMaximized(), "initial maximized state was omitted");
            root.modal()->maximize(false);
            root.modal()->fullscreen(true);
            require(root.native()->isFullScreen(), "imperative fullscreen did not apply");
            root.modal()->fullscreen(false);
            root.redraw();
            require(!root.native()->isMaximized() && !root.native()->isFullScreen(),
                    "redraw reapplied initial window flags");

            std::ostringstream errors;
            auto previous = std::cerr.rdbuf(errors.rdbuf());
            root.fail = true;
            root.native()->reject();
            root.fail = false;
            std::cerr.rdbuf(previous);
            require(errors.str().find("modal builder failure") != std::string::npos && root.modal()->ui,
                    "native close let a render exception escape Qt or lost the previous tree");
            root.redraw();
            settle();
            root.afterOpen = []() { throw std::runtime_error("open callback failure"); };
            previous = std::cerr.rdbuf(errors.rdbuf());
            second = 1;
            settle();
            std::cerr.rdbuf(previous);
            require(errors.str().find("open callback failure") != std::string::npos,
                    "queued callback exception escaped Qt or was not reported");
            root.afterOpen = {};
        }
        {
            // Destroying a view cancels its pending callbacks and captures.
            first = 1;
            auto root = std::unique_ptr<Root>(new Root(argc, argv, first, second));
            auto observer = std::weak_ptr<int>(root->token);
            int callbacks = 0;
            root->afterOpen = [&]() { ++callbacks; };
            root->afterClose = [&]() { ++callbacks; };
            root->redraw();
            QPointer<QWidget> modal = root->native();
            root.reset();
            settle();
            require(callbacks == 0 && observer.expired() && modal.isNull(),
                    "view destruction retained native dialogs/captures or invoked pending callbacks");
        }
        {
            int callbacks = 0;
            NestedRoot root(argc, argv, callbacks);
            root.redraw();
            QPointer<QWidget> native = static_cast<QWidget *>(find(root, "nested-modal")->outer());
            root.include = false;
            root.redraw();
            settle();
            require(native.isNull() && callbacks == 0,
                    "nested view removal invoked callbacks into destroyed local State");
        }
        {
            CPPUI::State<int> status{1};
            External root(argc, argv, status);
            root.redraw();
            auto native = static_cast<QDialog *>(find(root, "modal")->outer());
            require(native->isVisible(), "external getter did not drive modal visibility");
            native->reject();
            settle();
            require(static_cast<int>(status) == -7 && !native->isVisible(), "explicit off-value was lost");
        }
        {
            CPPUI::State<bool> status{false};
            Routing root(argc, argv, status);
            root.redraw();
            auto widget = [&](const char * id) { return static_cast<QWidget *>(find(root, id)->outer()); };
            auto window = static_cast<QMainWindow *>(widget("window"));
            auto splitter = static_cast<QSplitter *>(widget("splitter"));
            auto tabs = static_cast<QTabWidget *>(widget("tabs"));
            require(window->centralWidget() == splitter && splitter->count() == 2 && tabs->count() == 1,
                    "out-of-order modals replaced container content");
            for(auto id: {"top", "window-modal", "splitter-modal", "tab-modal", "nested-modal"}) {
                require(!widget(id)->isVisible(), "a native parent showed a closed modal");
            }
            QPointer<QWidget> nested = widget("nested-modal");
            require(nested->parentWidget() == widget("tab-modal"), "nested modal lost native ownership");
            delete window;
            require(nested.isNull() && !find(root, "nested-modal")->outer(),
                    "parent deletion did not invalidate nested modal observers");
        }
        settle();
        require(QApplication::topLevelWidgets().empty(), "modal tests left top-level widgets");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

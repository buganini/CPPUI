#include "cppui/qt/views.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/textfield.hpp"
#include "cppui/qt/button.hpp"
#include "cppui/qt/checkbox.hpp"
#include <QApplication>
#include <QEvent>
#include <QLineEdit>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    int editors = 0;
    class Editor: public CPPUI::Qt::QtView {
        public:
        std::string name;
        CPPUI::State<std::string> text{std::string("original")};
        int local = 0;
        explicit Editor(const std::string & name): name(name) { ++editors; }
        ~Editor() override { --editors; }
        void content() override {
            if(name == "empty") return;
            CPPUI::Label(name);
            CPPUI::TextField(text);
        }
        QLineEdit * field() { return static_cast<QLineEdit *>(children->at(1)->outer()); }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        std::vector<std::string> keys{"a", "b", "empty"};
        CPPUI::State<CPPUI::OptionalText> first{CPPUI::OptionalText("a")}, second{CPPUI::OptionalText("b")};
        bool show = true, external = false, other = false, missingID = false;
        int writes = 0;
        std::function<void()> action;
        void content() override {
            for(auto window: CPPUI::Window()) if(show) {
                auto state = other ? &second : &first;
                CPPUI::Binding<CPPUI::OptionalText> binding([state]() { return static_cast<CPPUI::OptionalText>(*state); },
                    [this, state](const CPPUI::OptionalText & key) {
                        ++writes; auto invoke = action; *state = key; if(invoke) invoke();
                    });
                auto & container = external ? CPPUI::Views(binding) : CPPUI::Views();
                for(auto scope: container) for(const auto & key: keys) {
                    auto editor = new Editor(key);
                    if(!missingID) editor->id(key);
                }
            }
        }
        CPPUI::Qt::Views * views() { return static_cast<CPPUI::Qt::Views *>(children->front()->children->front()); }
        QStackedWidget * native() { return static_cast<QStackedWidget *>(views()->ui); }
        Editor * editor(int index) { return static_cast<Editor *>(views()->children->at(index)->children->front()); }
    };
    class SelectionRoot: public CPPUI::Qt::QtView {
        public:
        CPPUI::State<CPPUI::OptionalText> page{CPPUI::OptionalText("a")};
        CPPUI::State<bool> checked{false};
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) {
                CPPUI::Button("a").click([this]() { page = CPPUI::OptionalText("a"); });
                CPPUI::Button("b").click([this]() { page = CPPUI::OptionalText("b"); });
                CPPUI::Checkbox("redraw", checked);
                for(auto views: CPPUI::Views(page)) {
                    CPPUI::Label("first").id("a");
                    CPPUI::Label("second").id("b");
                }
            }
        }
    };
    void buttonSelectionAfterRedraw() {
        SelectionRoot root; root.sync(); settle();
        auto window = static_cast<QWidget *>(root.children->front()->outer());
        auto stack = window->findChild<QStackedWidget *>();
        for(int repeat = 0; repeat < 8; ++repeat) {
            root.sync(); settle();
            const auto key = repeat % 2 ? "a" : "b";
            QPushButton * button = nullptr;
            for(auto candidate: window->findChildren<QPushButton *>())
                if(candidate->text() == key) button = candidate;
            require(button, "selection button was lost after redraw");
            button->click(); settle();
            auto checkbox = window->findChild<QCheckBox *>();
            require(checkbox, "redraw checkbox was lost");
            checkbox->click(); settle();
            root.sync(); settle();
            const auto selected = static_cast<CPPUI::OptionalText>(root.page);
            require(selected && *selected == key
                && static_cast<bool>(root.checked) == (repeat % 2 == 0),
                "button or checkbox callback lost its current receiver after redraw");
            require(window->findChild<QStackedWidget *>() == stack
                && stack->currentIndex() == (repeat % 2 ? 1 : 2),
                "button/checkbox redraw replaced Views or lost the selected page");
        }
    }
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        {
            Root root; root.sync();
            auto stack = root.native();
            require(stack->count() == 4 && stack->currentIndex() == 0 && editors == 3, "internal model did not start empty or pages were not grouped");
            require(stack->widget(1)->findChildren<QLabel *>().size() == 1
                && stack->widget(1)->findChildren<QLineEdit *>().size() == 1
                && stack->widget(3)->layout()->count() == 0, "multi-root or empty view was not retained as one page");
            auto editor = root.editor(0);
            QPointer<QWidget> removed = stack->widget(2);
            auto field = editor->field(); editor->local = 42;
            field->setText("draft"); field->editingFinished(); settle();
            root.views()->select("a"); settle();
            auto page = stack->currentWidget();
            require(stack->currentIndex() == 1, "internal selection did not update");
            root.views()->select("b"); settle();
            editor->text = std::string("hidden draft"); settle();
            require(stack->currentIndex() == 2 && editor->field() == field && field->text() == "hidden draft",
                "hidden view could not redraw independently");
            field->setText("draft"); field->editingFinished(); settle();
            root.views()->select("a"); settle();
            root.keys = {"empty", "a"}; root.sync(); settle();
            require(root.native() == stack && stack->currentWidget() == page && stack->currentIndex() == 2
                && root.editor(1) == editor && editor->local == 42 && editor->field() == field && field->text() == "draft"
                && removed.isNull() && editors == 2, "switch/reorder/removal lost retained view state or leaked pages");
            editor->redraw(); settle();
            require(field->text() == "draft", "independent hidden-view redraw lost the editor buffer");
            root.views()->select(); settle();
            require(stack->currentIndex() == 0, "clearing internal selection did not show the empty page");
            root.views()->select("unknown"); settle();
            require(stack->currentIndex() == 0, "unknown selection did not show the empty page");
            root.keys.push_back("unknown"); root.sync();
            require(stack->currentIndex() == 3, "returning selected key was not restored");
            root.keys.clear(); root.sync(); settle();
            require(stack->count() == 1 && editors == 0, "empty Views retained declared pages");
            root.keys = {"a"}; root.sync();
            for(const auto & invalid: std::vector<std::vector<std::string>>{{""}, {"a", "a"}}) {
                root.keys = invalid;
                bool rejected = false;
                try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && root.native() == stack && stack->count() == 2 && editors == 1
                    && CPPUI::getStack()->empty() && root.frames.empty(), "invalid page IDs corrupted mounted UI or construction scopes");
            }
            root.keys = {"a"}; root.missingID = true;
            bool rejected = false;
            try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && editors == 1 && stack->count() == 2, "missing page ID was accepted or leaked declarations");
            root.missingID = false; root.sync();
            rejected = false;
            try { root.views()->select(""); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && stack->currentIndex() == 0, "invalid imperative selection damaged the container");
        }
        settle(); require(editors == 0, "internal Views teardown retained views");
        {
            Root root; root.external = true; root.sync();
            auto stack = root.native();
            require(stack->currentIndex() == 1 && root.writes == 0, "bound selection was not read silently");
            root.first = CPPUI::OptionalText("b"); settle();
            require(stack->currentIndex() == 2 && root.writes == 0, "state selection caused feedback");
            root.views()->select("a"); settle();
            require(*static_cast<CPPUI::OptionalText>(root.first) == "a" && root.writes == 1, "select did not write the bound model once");
            root.other = true; root.sync(); root.views()->select(); settle();
            require(!static_cast<CPPUI::OptionalText>(root.second) && *static_cast<CPPUI::OptionalText>(root.first) == "a"
                && stack->currentIndex() == 0 && root.writes == 2, "rebind wrote the old model or failed to clear selection");
            root.second = CPPUI::OptionalText("missing"); settle();
            root.keys.clear(); root.sync();
            require(stack->currentIndex() == 0 && *static_cast<CPPUI::OptionalText>(root.second) == "missing", "absent pages rewrote the selection");
            root.keys = {"missing"}; root.sync();
            require(stack->currentIndex() == 1, "bound missing key was not restored after insertion");
            root.second = CPPUI::OptionalText("");
            bool rejected = false;
            try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && stack->currentIndex() == 1 && editors == 1, "invalid bound selection damaged mounted content");
            root.second = CPPUI::OptionalText("missing"); settle();
            root.action = [&]() { root.show = false; root.sync(); };
            QPointer<QWidget> removed = stack;
            root.views()->select(); settle();
            require(removed.isNull() && editors == 0, "selection setter self-removal retained pages");
        }
        {
            std::unique_ptr<Root> root(new Root()); root->external = true; root->sync();
            root->action = [&root]() { root.reset(); };
            root->views()->select("b"); settle();
            require(!root && editors == 0, "selection setter could not destroy its owning view");
        }
        {
            Root root; root.sync();
            auto declaration = root.views();
            delete root.native();
            require(!declaration->ui, "external native deletion left stale Views access");
        }
        buttonSelectionAfterRedraw();
        settle();
        require(editors == 0 && app.topLevelWidgets().isEmpty(), "Views teardown leaked native roots or declarations");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

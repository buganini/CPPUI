#include "cppui/qt/textfield.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/button.hpp"
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        for(int i = 0; i < 4; ++i) QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<std::string> first{std::string("first")}, second{std::string("second")}, draft{std::string()};
        bool other = false, show = true, failGet = false, throwSet = false;
        int writes = 0, inputs = 0, changes = 0, version = 1;
        std::function<void()> onInput, onCommit;
        std::string seen;
        std::string placeholder = "Enter text";
        bool setPlaceholder = true;
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) {
                if(show) {
                    auto state = other ? &second : &first;
                    auto binding = CPPUI::Binding<std::string>([this, state]() {
                        if(failGet) throw std::runtime_error("expected getter failure");
                        return static_cast<std::string>(*state);
                    }, [this, state](const std::string & text) {
                        ++writes;
                        if(throwSet) throw std::runtime_error("expected setter failure");
                        if(text != "reject") *state = text;
                    });
                    const auto current = version;
                    auto & field = CPPUI::TextField(binding, draft);
                    if(setPlaceholder) field.placeholder(std::string(placeholder));
                    field.input([this, current]() {
                        inputs += current; auto invoke = onInput; if(invoke) invoke();
                    }).change([this, current]() {
                        changes += current; auto invoke = onCommit; if(invoke) invoke();
                    });
                }
                CPPUI::Button("read model").click([this]() { seen = static_cast<std::string>(other ? second : first); });
            }
        }
        QLineEdit * editor() { return static_cast<QLineEdit *>(children->front()->children->front()->children->front()->outer()); }
        QPushButton * button() { return static_cast<QPushButton *>(children->front()->children->front()->children->back()->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync(); settle();
            auto editor = root.editor();
            require(editor->placeholderText() == "Enter text", "TextField lost its declared placeholder");
            require(editor->text() == "first" && static_cast<std::string>(root.draft) == "first" && root.writes == 0 && root.inputs == 0, "initial model/draft synchronization produced feedback");
            int external = 0;
            QObject::connect(editor, &QLineEdit::textChanged, [&]() { ++external; });
            editor->setText("unfinished"); settle();
            require(static_cast<std::string>(root.first) == "first" && static_cast<std::string>(root.draft) == "unfinished"
                && root.inputs == 1 && root.writes == 0, "typing committed the model instead of its draft");
            editor->setCursorPosition(2);
            for(int i = 0; i < 4; ++i) {
                root.placeholder = i == 1 ? "" : "Updated hint";
                root.setPlaceholder = i != 3;
                root.sync(); settle();
                require(root.editor() == editor && editor->placeholderText().toStdString() == (root.setPlaceholder ? root.placeholder : "")
                    && editor->text() == "unfinished" && editor->cursorPosition() == 2,
                    "placeholder update/default reset changed native identity, draft or cursor");
                require(static_cast<std::string>(root.first) == "first" && static_cast<std::string>(root.draft) == "unfinished"
                    && root.inputs == 1 && root.writes == 0 && root.changes == 0,
                    "placeholder update changed the model or emitted feedback");
            }
            root.first = std::string("outside"); root.version = 2; root.sync(); settle();
            require(root.editor() == editor && editor->text() == "unfinished", "redraw overwrote an unfinished edit");
            editor->editingFinished(); settle();
            require(static_cast<std::string>(root.first) == "unfinished" && root.writes == 1 && root.changes == 2 && external == 1, "commit feedback or stale callback after reuse");
            root.first = std::string("programmatic"); settle();
            require(editor->text() == "programmatic" && root.inputs == 1 && root.writes == 1, "programmatic model update emitted user input");
            editor->setText("new binding");
            root.other = true; root.sync();
            editor->editingFinished(); settle();
            require(static_cast<std::string>(root.second) == "new binding" && static_cast<std::string>(root.first) == "programmatic", "commit used a retired binding");
            editor->setText("reject"); editor->editingFinished(); settle();
            require(editor->text() == "new binding" && static_cast<std::string>(root.second) == "new binding", "rejected adapter value did not restore the model text");
            root.failGet = true;
            bool failed = false; try { root.sync(); } catch(const std::runtime_error &) { failed = true; }
            require(failed && root.editor() == editor, "getter failure replaced the mounted editor");
            root.failGet = false; root.sync();
            root.throwSet = true; editor->setText("throws"); editor->editingFinished();
            require(static_cast<std::string>(root.second) == "new binding", "setter exception changed the model");
            root.throwSet = false;
            auto window = static_cast<QWidget *>(root.children->front()->outer());
            window->activateWindow(); editor->setFocus(); settle();
            editor->setText("before button"); root.button()->click(); settle();
            require(root.seen == "before button", "button callback ran before editor model commit");
            QPointer<QWidget> removed = editor;
            root.onCommit = [&]() { root.show = false; root.sync(); };
            root.sync(); editor->setText("remove"); editor->editingFinished(); settle();
            require(removed.isNull() && static_cast<std::string>(root.second) == "remove", "commit self-removal retained native content");
            root.onCommit = {}; root.show = true; root.sync(); settle();
            removed = root.editor();
            root.onInput = [&]() { root.show = false; root.sync(); };
            root.sync(); root.editor()->setText("remove on input"); settle();
            require(removed.isNull(), "input self-removal retained native content");
            root.onInput = {}; root.show = true; root.sync(); settle();
            auto declaration = static_cast<CPPUI::Qt::TextField *>(root.children->front()->children->front()->children->front());
            delete root.editor(); require(!declaration->ui, "external editor deletion left stale access");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "TextField teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

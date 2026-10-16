#include "cppui/qt/combobox.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<int> index{-1};
        CPPUI::State<std::string> text{std::string()};
        bool editable = false, useIndex = true, useText = true, show = true, reverse = false;
        CPPUI::State<bool> disabled{false};
        bool setDisabled = true;
        int count = 3, writes = 0, changes = 0;
        std::string caption = "First";
        std::function<void()> action;
        using Application::Application;
        void item(int i) { CPPUI::ComboBoxItem(i == 0 ? caption : "Same", std::to_string(i)).id(std::to_string(i)); }
        void content() override {
            for(auto window: CPPUI::Window()) if(show) {
                CPPUI::Binding<std::string> binding([this]() { return static_cast<std::string>(text); }, [this](const std::string & value) {
                    ++writes; auto invoke = action; text = value; if(invoke) invoke();
                });
                auto & combo = useIndex
                    ? (useText ? CPPUI::ComboBox(editable, index, binding) : CPPUI::ComboBox(editable, index))
                    : (useText ? CPPUI::ComboBox(editable, binding) : CPPUI::ComboBox(editable));
                combo.change([this]() { ++changes; });
                if(setDisabled) combo.disabled(disabled);
                for(auto scope: combo) {
                    if(reverse) for(int i = count - 1; i >= 0; --i) item(i);
                    else for(int i = 0; i < count; ++i) item(i);
                }
            }
        }
        QComboBox * native() { return static_cast<QComboBox *>(children->front()->children->front()->outer()); }
    };
    class Items: public CPPUI::Qt::QtView {
        CPPUI::State<int> & count;
        public:
        explicit Items(CPPUI::State<int> & count): count(count) {}
        void content() override {
            for(int i = 0; i < count; ++i) CPPUI::ComboBoxItem("Item " + std::to_string(i), std::to_string(i));
        }
    };
    class Nested: public CPPUI::Application {
        public:
        CPPUI::State<int> count{2};
        CPPUI::State<std::string> value{std::string("1")};
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto combo: CPPUI::ComboBox(false, value)) new Items(count);
        }
        QComboBox * native() { return static_cast<QComboBox *>(children->front()->children->front()->outer()); }
    };
    class UnboundNested: public CPPUI::Application {
        public:
        CPPUI::State<int> firstCount{1}, secondCount{1};
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto combo: CPPUI::ComboBox()) {
                (new Items(firstCount))->id("first");
                (new Items(secondCount))->id("second");
            }
        }
        QComboBox * native() { return static_cast<QComboBox *>(children->front()->children->front()->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        for(bool editable: {false, true}) {
            Root root(argc, argv); root.editable = editable; root.sync();
            auto combo = root.native();
            for(int i = 0; i < 4; ++i) {
                root.setDisabled = i != 3;
                root.disabled = i != 1; settle();
                require(combo->isEnabled() == (!root.setDisabled || !static_cast<bool>(root.disabled)),
                    "reactive ComboBox disabled update/default reset was not applied");
                root.index = i % 3; root.text = std::to_string(i % 3); settle();
                require(root.native() == combo && combo->isEditable() == editable
                    && combo->currentIndex() == i % 3 && combo->currentText() == combo->itemText(i % 3),
                    "disabled ComboBox lost identity, editable mode or model updates");
                require(root.writes == 0 && root.changes == 0, "ComboBox option/model updates emitted feedback");
            }
        }
        {
            Root root(argc, argv); root.sync();
            auto combo = root.native();
            require(combo->count() == 3 && combo->currentIndex() == -1 && root.writes == 0, "initial negative index selected an item or caused feedback");
            combo->currentIndexChanged(-1); settle();
            require(static_cast<std::string>(root.text).empty(), "negative index used the last item value");
            int external = 0;
            QObject::connect(combo, &QComboBox::currentIndexChanged, [&]() { ++external; });
            combo->setCurrentIndex(2); settle();
            require(static_cast<int>(root.index) == 2 && static_cast<std::string>(root.text) == "2" && root.writes == 2 && external == 1,
                "selection lost distinct values or duplicated its commit");
            root.reverse = true; root.sync();
            require(combo->currentIndex() == 2 && combo->itemData(0).toString() == "2" && root.writes == 2, "keyed item reorder did not restore bound index silently");
            root.caption = "Renamed"; root.sync();
            require(combo->itemText(2) == "Renamed", "retained item label was not refreshed");
            root.useIndex = false; root.text = std::string("1"); root.sync();
            require(combo->currentIndex() == 1, "text-only binding did not select by value");
            root.text = std::string("unknown"); settle();
            require(combo->currentIndex() == -1, "unmatched value did not clear readonly selection");
            root.editable = true; root.sync();
            combo->setEditText("typed"); settle();
            require(static_cast<std::string>(root.text) == "typed" && combo->currentText() == "typed", "editable text binding lost free text");
            root.useIndex = true; root.index = -1; root.sync();
            combo->setEditText("draft"); settle();
            require(combo->currentText() == "draft", "unchanged index/text declarations overwrote editable input");
            root.count = 0; root.sync();
            require(combo->count() == 0 && combo->currentIndex() == -1, "empty items retained a stale index");
            root.count = 3; root.reverse = false; root.editable = false; root.index = 1; root.sync();
            require(combo->count() == 3 && combo->currentIndex() == 1, "repopulating items lost the bound selection");
            const int writes = root.writes;
            for(int i = 0; i < 12; ++i) root.sync();
            combo->setCurrentIndex(0); settle();
            require(root.writes == writes + 1 && root.native() == combo, "redraw accumulated model writes or remounted the combo");
            root.action = [&]() { root.show = false; root.sync(); };
            QPointer<QWidget> removed = combo;
            combo->setCurrentIndex(1); settle();
            require(removed.isNull(), "selection callback removal retained the native combo");
            root.action = {}; root.show = true; root.sync();
            auto declaration = static_cast<CPPUI::Qt::ComboBox *>(root.children->front()->children->front());
            delete root.native(); require(!declaration->ui, "external combo deletion left stale access");
        }
        {
            Root root(argc, argv);
            root.editable = true; root.index = 0; root.text = std::string("0"); root.sync();
            auto combo = root.native();
            require(combo->currentText() == "First", "editable selection displayed its raw value instead of its label");
            combo->setEditText("typed"); settle();
            require(static_cast<int>(root.index) == -1 && static_cast<std::string>(root.text) == "typed"
                && combo->currentIndex() == -1 && combo->currentText() == "typed" && root.writes == 1 && root.changes == 1,
                "free text did not clear index and commit once with both bindings");
            root.reverse = true; root.caption = "Renamed"; root.sync();
            require(combo->currentText() == "typed" && root.writes == 1, "item changes discarded editable free text or caused feedback");
            combo->setCurrentIndex(1); settle();
            require(static_cast<int>(root.index) == 1 && static_cast<std::string>(root.text) == "1"
                && combo->currentText() == "Same" && root.writes == 2 && root.changes == 2,
                "editable selection committed its label or emitted duplicate changes");
            root.useIndex = false; root.text = std::string("0"); root.sync();
            require(combo->currentIndex() == 2 && combo->currentText() == "Renamed", "text-only editable binding did not resolve the stored value to its label");
            root.action = [&]() { root.show = false; root.sync(); };
            QPointer<QWidget> removed = combo;
            combo->setEditText("remove"); settle();
            require(removed.isNull() && static_cast<std::string>(root.text) == "remove", "text callback removal lost the commit or retained native access");
        }
        {
            Root root(argc, argv);
            root.editable = true; root.useText = false; root.index = 0; root.sync();
            auto combo = root.native();
            combo->setEditText("local draft"); settle();
            require(static_cast<int>(root.index) == -1 && combo->currentText() == "local draft" && root.changes == 1,
                "index-only binding discarded its local edit buffer");
            root.reverse = true;
            for(int i = 0; i < 12; ++i) root.sync();
            require(combo->currentIndex() == -1 && combo->currentText() == "local draft" && root.changes == 1,
                "index-only redraw reset the edit buffer or emitted changes");
            combo->setCurrentIndex(0); settle();
            require(static_cast<int>(root.index) == 0 && root.changes == 2, "index-only selection committed twice");
        }
        {
            Root root(argc, argv);
            root.useIndex = false; root.useText = false; root.sync();
            auto combo = root.native();
            combo->setCurrentIndex(0); settle();
            root.reverse = true; root.caption = "Renamed"; root.sync();
            require(combo->currentIndex() == 2 && combo->currentText() == "Renamed" && root.changes == 1,
                "unbound selection did not follow its renamed keyed item");
            combo->setCurrentIndex(0); settle();
            root.count = 2; root.sync();
            require(combo->currentIndex() == -1, "removed unbound selection moved to an unrelated item");
            root.count = 3; root.sync();
            require(combo->currentIndex() == 0 && combo->currentData().toString() == "2", "returning keyed item did not restore unbound selection");
            root.editable = true; root.sync();
            combo->setEditText("unbound draft"); settle();
            root.reverse = false; root.sync();
            require(combo->currentIndex() == -1 && combo->currentText() == "unbound draft", "unbound editable buffer was lost on reorder");
        }
        {
            UnboundNested root(argc, argv); root.sync();
            auto combo = root.native();
            combo->setCurrentIndex(1); settle();
            root.firstCount = 2; settle();
            require(combo->count() == 3 && combo->currentIndex() == 2,
                "unbound selection collided with an equal ID in another virtual scope");
            root.firstCount = 0; settle();
            require(combo->count() == 1 && combo->currentIndex() == 0, "independent sibling removal lost selected virtual item");
        }
        {
            Nested root(argc, argv); root.sync();
            auto combo = root.native();
            require(combo->currentIndex() == 1, "virtual items did not select by stored value");
            root.count = 1; settle();
            require(combo->count() == 1 && combo->currentIndex() == -1, "independent item removal retained invalid selection");
            root.count = 0; settle();
            root.count = 3; settle();
            require(combo->count() == 3 && combo->currentIndex() == 1, "independent item repopulation lost selection");
            combo->setCurrentIndex(2); settle();
            require(static_cast<std::string>(root.value) == "2", "virtual item callback read stale values");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "combo teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

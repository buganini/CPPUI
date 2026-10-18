#include "cppui/qt/tab.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include <QApplication>
#include <QEvent>
#include <map>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    int declarations = 0;
    class TrackedTab: public CPPUI::Qt::Tab {
        public:
        explicit TrackedTab(const std::string & label): CPPUI::Qt::Tab(label) { ++declarations; }
        ~TrackedTab() override { --declarations; }
    };
    class Pair: public CPPUI::Qt::QtView {
        public:
        void content() override {
            CPPUI::Label("pair 1");
            CPPUI::Label("pair 2");
        }
    };
    enum Kind { Single, VirtualPair, Empty, Grouped };
    struct Page { std::string key; std::string title; Kind kind = Single; };
    class Root: public CPPUI::Qt::QtView {
        public:
        bool show = true, external = false, other = false, invalidChild = false;
        CPPUI::State<CPPUI::OptionalText> first{CPPUI::OptionalText("b")}, second{CPPUI::OptionalText("a")};
        int writes = 0;
        std::function<void()> action;
        CPPUI::Side position = CPPUI::Side::TOP;
        std::vector<Page> pages{{"a", "A"}, {"b", "B"}, {"c", "C"}};
        ~Root() {
            clearChildren();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        void content() override {
            if(!show) return;
            auto state = other ? &second : &first;
            CPPUI::Binding<CPPUI::OptionalText> binding([state]() { return static_cast<CPPUI::OptionalText>(*state); },
                [this, state](const CPPUI::OptionalText & value) {
                    ++writes; auto invoke = action; *state = value; if(invoke) invoke();
                });
            for(auto tabs: external ? CPPUI::Tabs(position, binding) : CPPUI::Tabs(position)) {
                if(invalidChild) CPPUI::Label("invalid");
                for(const auto & page: pages) {
                    for(auto tab: (new TrackedTab(std::string(page.title)))->id(page.key)) {
                        switch(page.kind) {
                            case Single: CPPUI::Label(page.key); break;
                            case VirtualPair: CPPUI::SubView<Pair>(); break;
                            case Empty: break;
                            case Grouped:
                                for(auto box: CPPUI::VBox()) {
                                    CPPUI::Label("group 1");
                                    CPPUI::Label("group 2");
                                }
                                break;
                        }
                    }
                }
            }
        }
        CPPUI::Qt::Tabs * tabs() { return static_cast<CPPUI::Qt::Tabs *>(children->front()); }
        QTabWidget * native() { return static_cast<QTabWidget *>(children->front()->outer()); }
        void checkSinglePages() {
            auto tabs = native();
            require(tabs->count() == static_cast<int>(pages.size()), "incorrect single-child page count");
            for(int i = 0; i < tabs->count(); ++i) {
                auto label = tabs->widget(i)->findChild<QLabel *>();
                require(label && label->text().toStdString() == pages[i].key
                    && tabs->tabText(i).toStdString() == pages[i].title, "page order or updated tab title is wrong");
            }
            require(declarations == static_cast<int>(pages.size()), "redraw retained old Tab declarations");
        }
    };
    void ordinaryTabs() {
        int destroyed = 0;
        int removed = 0;
        {
            Root root;
            root.sync();
            auto tabs = root.native();
            QObject::connect(tabs, &QObject::destroyed, [&]() { ++destroyed; });
            tabs->resize(400, 240);
            tabs->show();
            settle();
            root.checkSinglePages();
            auto declaration = root.children->front()->children->front();
            require(!declaration->is_virtual && declaration->outer() == tabs->widget(0)
                && tabs->widget(0)->contentsMargins().isNull(), "Tab did not own a zero-padding page");
            std::map<std::string, QWidget *> retained;
            for(int i = 0; i < tabs->count(); ++i) retained[root.pages[i].key] = tabs->widget(i);
            tabs->setCurrentIndex(1);
            for(int repeat = 0; repeat < 5; ++repeat) {
                root.sync();
                settle();
                root.checkSinglePages();
                require(root.native() == tabs && tabs->currentWidget() == retained["b"],
                    "unchanged redraw replaced tabs or reset native selection");
            }
            root.pages[1].title = "Renamed";
            root.sync();
            root.checkSinglePages();
            require(tabs->currentWidget() == retained["b"], "title update changed selection");
            root.pages.insert(root.pages.begin(), {"new", "New"});
            root.sync();
            root.checkSinglePages();
            require(tabs->currentWidget() == retained["b"], "insertion lost the selected page");
            root.pages = {{"c", "C"}, {"a", "A"}, {"b", "B"}};
            root.sync();
            settle();
            root.checkSinglePages();
            require(tabs->widget(0) == retained["c"] && tabs->widget(1) == retained["a"]
                && tabs->widget(2) == retained["b"], "keyed move recreated tab contents");
            QObject::connect(retained["b"], &QObject::destroyed, [&]() { ++removed; });
            root.pages = {{"c", "C"}, {"a", "A"}};
            root.sync();
            settle();
            root.checkSinglePages();
            require(removed == 1, "removed page did not release its native widget");
            root.pages.clear();
            root.sync();
            settle();
            root.checkSinglePages();
            require(root.native() == tabs && tabs->currentIndex() == -1, "empty tabs lost native identity");
            root.pages = {{"a", "Again"}};
            root.position = CPPUI::Side::BOTTOM;
            root.sync();
            root.checkSinglePages();
            require(tabs->tabPosition() == QTabWidget::South, "south tab position was not applied");
            auto page = tabs->widget(0);
            for(const auto & position: std::map<CPPUI::Side, QTabWidget::TabPosition>{
                    {CPPUI::Side::TOP, QTabWidget::North}, {CPPUI::Side::BOTTOM, QTabWidget::South}, {CPPUI::Side::RIGHT, QTabWidget::East}, {CPPUI::Side::LEFT, QTabWidget::West}}) {
                root.position = position.first; root.sync();
                require(root.native() == tabs && tabs->tabPosition() == position.second
                    && tabs->currentWidget() == page, "tab placement change replaced native content or selection");
            }
            for(auto invalid: {static_cast<CPPUI::Side>(-1), static_cast<CPPUI::Side>(99)}) {
                root.position = invalid;
                bool rejected = false;
                try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && root.native() == tabs && tabs->currentWidget() == page
                    && declarations == 1, "invalid tab placement damaged mounted content");
            }
            root.position = CPPUI::Side::TOP;
            root.show = false;
            root.sync();
            settle();
            require(declarations == 0 && destroyed == 1, "Tabs removal did not release declarations/native container");
            root.show = true;
            root.sync();
            QObject::connect(root.native(), &QObject::destroyed, [&]() { ++destroyed; });
        }
        require(declarations == 0 && destroyed == 2, "root teardown retained Tabs/Tab");
    }
    void optionalSelection() {
        {
            Root root; root.pages = {{"a", "Alpha"}, {"", "Beta"}}; root.sync();
            auto native = root.native(); auto first = native->widget(0);
            root.tabs()->select("Beta"); settle();
            require(native->currentIndex() == 1, "internal label fallback selection failed");
            native->setCurrentIndex(0); settle(); root.sync();
            require(native->currentIndex() == 0, "native selection did not persist in the internal model");
            root.pages = {{"", "Beta"}, {"a", "Renamed"}}; root.sync();
            require(native->currentIndex() == 1 && native->currentWidget() == first, "internal key was lost on reorder/rename");
            root.tabs()->select("Renamed"); settle();
            require(native->currentIndex() == 0, "explicit ID did not override the label");
            root.pages = {{"", "Beta"}, {"Renamed", "Returned"}}; root.sync();
            require(native->currentIndex() == 1, "unknown internal key was rewritten instead of retained");
            root.tabs()->select(); settle();
            require(native->currentIndex() == 0, "empty selection did not display the first tab");
            root.pages = {{"dup", "First"}, {"dup", "Second"}, {"", "Same"}, {"", "Same"}}; root.sync();
            root.tabs()->select("dup"); settle();
            require(native->currentIndex() == 0, "duplicate IDs did not choose the first match");
            root.tabs()->select("Same"); settle();
            require(native->currentIndex() == 2, "duplicate labels did not choose the first match");
            native->setCurrentIndex(3); settle();
            require(native->currentIndex() == 2, "native selection did not resolve duplicate keys to the first match");
            root.tabs()->select(""); settle();
            require(native->currentIndex() == 0, "empty-string Tabs selection was rejected");
            root.pages = {{"", ""}}; root.sync();
            require(native->currentIndex() == 0, "empty label could not be selected");
            root.invalidChild = true;
            bool rejected = false;
            try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && native->count() == 1 && declarations == 1, "invalid Tabs child damaged mounted pages");
        }
        settle();
        {
            Root root; root.external = true; root.sync();
            auto native = root.native();
            require(native->currentIndex() == 1 && root.writes == 0, "bound initial selection caused feedback");
            root.first = CPPUI::OptionalText("c"); settle();
            require(native->currentIndex() == 2 && root.writes == 0, "State did not select the bound tab silently");
            native->setCurrentIndex(0); settle();
            require(*static_cast<CPPUI::OptionalText>(root.first) == "a" && root.writes == 1, "native selection did not write exactly once");
            root.other = true; root.sync(); root.tabs()->select("b"); settle();
            require(*static_cast<CPPUI::OptionalText>(root.first) == "a" && *static_cast<CPPUI::OptionalText>(root.second) == "b"
                && native->currentIndex() == 1 && root.writes == 2, "selection wrote a stale model after rebinding");
            root.pages.clear(); root.sync();
            require(native->currentIndex() == -1 && root.writes == 2, "empty Tabs rewrote the model");
            root.pages = {{"a", "Alpha"}}; root.sync();
            require(native->currentIndex() == 0 && *static_cast<CPPUI::OptionalText>(root.second) == "b", "missing key was not preserved");
            root.pages.push_back({"b", "Beta"}); root.sync();
            require(native->currentIndex() == 1 && root.writes == 2, "returning selected key was not restored silently");
            root.tabs()->select(); settle();
            require(!static_cast<CPPUI::OptionalText>(root.second) && native->currentIndex() == 0 && root.writes == 3, "clearing optional binding lost None");
            root.external = false; root.sync();
            native->setCurrentIndex(1); settle();
            root.pages = {{"b", "Beta"}, {"a", "Alpha"}}; root.sync();
            require(native->currentIndex() == 0 && root.writes == 3, "external-to-internal rebinding lost native selection");
            root.external = true; root.sync();
            root.action = [&]() { root.show = false; root.sync(); };
            QPointer<QWidget> removed = native;
            native->setCurrentIndex(1); settle();
            require(removed.isNull() && declarations == 0, "native selection setter self-removal leaked Tabs");
        }
        {
            std::unique_ptr<Root> root(new Root()); root->external = true; root->sync();
            root->action = [&]() { root.reset(); };
            root->tabs()->select("c"); settle();
            require(!root && declarations == 0, "select could not safely destroy its owning view");
        }
    }
    void selectionAfterRedraw() {
        std::vector<int> seen;
        Root root;
        root.external = true;
        root.pages = {{"alpha", "Alpha"}, {"", "Beta"}};
        root.first = CPPUI::OptionalText("alpha");
        root.sync();
        auto native = root.native();
        auto listener = QObject::connect(native, &QTabWidget::currentChanged, native,
            [&seen](int index) { seen.push_back(index); });
        for(int repeat = 0; repeat < 8; ++repeat) {
            // Retire declarations before using the retained native signal source.
            root.sync(); settle();
            root.checkSinglePages();
            const int index = (repeat + 1) % 2;
            const std::string key = index ? "Beta" : "alpha";
            native->setCurrentIndex(index); settle();
            const auto selected = static_cast<CPPUI::OptionalText>(root.first);
            require(selected && *selected == key
                && root.writes == repeat + 1, "native selection lost its current receiver or wrote more than once after redraw");
            root.sync(); settle();
            require(root.native() == native && native->currentIndex() == index,
                "native selection after redraw did not persist in the bound model");
            require(seen.size() == static_cast<std::size_t>(repeat + 1) && seen.back() == index,
                "redraw disconnected or duplicated an external selection listener");
        }
        QObject::disconnect(listener);
    }
    void virtualPages() {
        int removed = 0;
        {
            Root root;
            root.pages = {{"multi", "Multi", VirtualPair}, {"last", "Last"}};
            root.sync();
            auto tabs = root.native();
            auto pair = tabs->widget(0);
            auto labels = pair->findChildren<QLabel *>();
            auto first = labels.at(0);
            auto second = labels.at(1);
            auto last = tabs->widget(1);
            require(tabs->count() == 2 && first && second && first->text() == "pair 1"
                && second->text() == "pair 2", "multi-root view was not grouped into one Tab page");
            require(tabs->tabText(0) == "Multi" && tabs->tabText(1) == "Last",
                "multi-child Tab label did not match its page");
            for(int repeat = 0; repeat < 5; ++repeat) {
                root.pages = {{"last", "Last"}, {"multi", "Multi", VirtualPair}};
                root.sync();
                require(tabs->widget(0) == last && tabs->widget(1) == pair && pair->findChildren<QLabel *>() == labels,
                    "moving a virtual Tab/view subtree lost native order/identity");
                require(tabs->tabText(0) == "Last" && tabs->tabText(1) == "Multi",
                    "moved pages failed to find their ancestor Tab label");
                root.pages = {{"multi", "Multi", VirtualPair}, {"last", "Last"}};
                root.sync();
                require(tabs->widget(0) == pair && tabs->widget(1) == last && pair->findChildren<QLabel *>() == labels,
                    "reverse virtual move lost native order/identity");
                require(declarations == 2, "virtual moves retained old Tab declarations");
            }
            QObject::connect(first, &QObject::destroyed, [&]() { ++removed; });
            QObject::connect(second, &QObject::destroyed, [&]() { ++removed; });
            root.pages = {{"empty", "Empty", Empty}, {"last", "Last"}};
            root.sync();
            settle();
            require(removed == 2 && tabs->count() == 2 && tabs->widget(1) == last
                && tabs->widget(0)->layout()->count() == 0,
                "empty Tab lost its page or removed virtual contents leaked");
            require(tabs->tabText(0) == "Empty" && tabs->tabText(1) == "Last", "empty Tab changed its sibling label");
            root.pages = {{"group", "Group", Grouped}};
            root.sync();
            settle();
            require(tabs->count() == 1 && tabs->widget(0)->findChildren<QLabel *>().size() == 2,
                "explicit VBox failed to group multiple controls into one page");
            delete root.native();
            require(!root.children->front()->outer(), "external native deletion left a stale Tabs observer");
        }
        require(declarations == 0, "virtual/empty Tab declarations leaked on teardown");
    }
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        ordinaryTabs();
        virtualPages();
        optionalSelection();
        selectionAfterRedraw();
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

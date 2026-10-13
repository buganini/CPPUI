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
        bool show = true;
        std::string position = "n";
        std::vector<Page> pages{{"a", "A"}, {"b", "B"}, {"c", "C"}};
        ~Root() {
            clearChildren();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        void content() override {
            if(show) for(auto tabs: CPPUI::Tabs(position)) {
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
        QTabWidget * native() { return static_cast<QTabWidget *>(children->front()->outer()); }
        void checkSinglePages() {
            auto tabs = native();
            require(tabs->count() == static_cast<int>(pages.size()), "incorrect single-child page count");
            for(int i = 0; i < tabs->count(); ++i) {
                auto label = qobject_cast<QLabel *>(tabs->widget(i));
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
            require(declaration->is_virtual && !declaration->ui, "Tab acquired a physical widget");
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
            root.position = "s";
            root.sync();
            root.checkSinglePages();
            require(tabs->tabPosition() == QTabWidget::South, "south tab position was not applied");
            auto page = tabs->widget(0);
            for(const auto & position: std::map<std::string, QTabWidget::TabPosition>{
                    {"n", QTabWidget::North}, {"s", QTabWidget::South}, {"e", QTabWidget::East}, {"w", QTabWidget::West}}) {
                root.position = position.first; root.sync();
                require(root.native() == tabs && tabs->tabPosition() == position.second
                    && tabs->currentWidget() == page, "tab placement change replaced native content or selection");
            }
            for(const auto & invalid: {"", "north", "invalid"}) {
                root.position = invalid;
                bool rejected = false;
                try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && root.native() == tabs && tabs->currentWidget() == page
                    && declarations == 1, "invalid tab placement damaged mounted content");
            }
            root.position = "n";
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
    void virtualPages() {
        int removed = 0;
        {
            Root root;
            root.pages = {{"multi", "Multi", VirtualPair}, {"last", "Last"}};
            root.sync();
            auto tabs = root.native();
            auto first = qobject_cast<QLabel *>(tabs->widget(0));
            auto second = qobject_cast<QLabel *>(tabs->widget(1));
            auto last = tabs->widget(2);
            require(tabs->count() == 3 && first && second && first->text() == "pair 1"
                && second->text() == "pair 2", "virtual Tab/view children were grouped or discarded");
            require(tabs->tabText(0) == "Multi" && tabs->tabText(1) == "Last" && tabs->tabText(2) == "Last",
                "multi-child label updates differ from pinned PUI postSync");
            for(int repeat = 0; repeat < 5; ++repeat) {
                root.pages = {{"last", "Last"}, {"multi", "Multi", VirtualPair}};
                root.sync();
                require(tabs->widget(0) == last && tabs->widget(1) == first && tabs->widget(2) == second,
                    "moving a virtual Tab/view subtree lost native order/identity");
                require(tabs->tabText(0) == "Last" && tabs->tabText(1) == "Multi" && tabs->tabText(2) == "Multi",
                    "moved pages failed to find their ancestor Tab label");
                root.pages = {{"multi", "Multi", VirtualPair}, {"last", "Last"}};
                root.sync();
                require(tabs->widget(0) == first && tabs->widget(1) == second && tabs->widget(2) == last,
                    "reverse virtual move lost native order/identity");
                require(declarations == 2, "virtual moves retained old Tab declarations");
            }
            QObject::connect(first, &QObject::destroyed, [&]() { ++removed; });
            QObject::connect(second, &QObject::destroyed, [&]() { ++removed; });
            root.pages = {{"empty", "Empty", Empty}, {"last", "Last"}};
            root.sync();
            settle();
            require(removed == 2 && tabs->count() == 1 && tabs->widget(0) == last,
                "empty Tab created a page or removed virtual pages leaked");
            require(tabs->tabText(0) == "Empty", "empty-Tab label update differs from pinned PUI postSync");
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
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

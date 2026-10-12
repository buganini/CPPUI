#include "cppui/qt/splitter.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/window.hpp"
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
    class Pair: public CPPUI::Qt::QtView {
        public:
        void content() override {
            CPPUI::Label("group 1");
            CPPUI::Label("group 2");
        }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        bool vertical = false;
        bool show = true;
        std::vector<std::string> panes{"a", "b", "c"};
        ~Root() {
            clearChildren();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        void content() override {
            for(auto window: CPPUI::Window()) {
                if(show) for(auto splitter: CPPUI::Splitter(vertical)) {
                    for(const auto & pane: panes) {
                        if(pane == "group") CPPUI::SubView<Pair>().id(pane);
                        else CPPUI::Label(pane).id(pane);
                    }
                }
            }
        }
        QSplitter * native() {
            return static_cast<QSplitter *>(children->front()->children->front()->outer());
        }
        void checkOrder() {
            int i = 0;
            for(const auto & pane: panes) {
                for(const auto & text: pane == "group" ? std::vector<std::string>{"group 1", "group 2"}
                                                       : std::vector<std::string>{pane}) {
                    auto label = qobject_cast<QLabel *>(native()->widget(i++));
                    require(label && label->text().toStdString() == text, "splitter native order differs from declarations");
                }
            }
            require(native()->count() == i, "splitter has extra native panes");
        }
    };
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        int destroyed = 0;
        {
            Root root;
            root.sync();
            auto window = static_cast<QWidget *>(root.children->front()->outer());
            window->resize(800, 600);
            window->show();
            settle();
            auto splitter = root.native();
            QObject::connect(splitter, &QObject::destroyed, &app, [&]() { ++destroyed; });
            require(splitter->orientation() == Qt::Horizontal, "default splitter must be horizontal");
            root.checkOrder();
            std::map<std::string, QWidget *> retained;
            for(int i = 0; i < splitter->count(); ++i) retained[root.panes[i]] = splitter->widget(i);
            splitter->setSizes({100, 250, 450});
            const auto sizes = splitter->sizes();
            require(sizes[0] < sizes[1] && sizes[1] < sizes[2], "test did not establish distinct pane sizes");
            for(int repeat = 0; repeat < 5; ++repeat) {
                root.sync();
                settle();
                require(root.native() == splitter && splitter->sizes() == sizes,
                        "unchanged redraw replaced the splitter or reset user sizes");
            }
            root.panes = {"c", "group", "a", "b"};
            root.sync();
            settle();
            root.checkOrder();
            require(splitter->widget(0) == retained["c"] && splitter->widget(3) == retained["a"]
                && splitter->widget(4) == retained["b"], "insertion/reorder replaced retained panes");
            auto group1 = splitter->widget(1);
            auto group2 = splitter->widget(2);
            root.panes = {"group", "b", "a", "c"};
            root.vertical = true;
            root.sync();
            settle();
            root.checkOrder();
            require(root.native() == splitter && splitter->orientation() == Qt::Vertical,
                    "orientation change did not reuse the splitter");
            require(splitter->widget(0) == group1 && splitter->widget(1) == group2,
                    "moving a virtual subtree replaced its native panes");
            int removed = 0;
            QObject::connect(retained["b"], &QObject::destroyed, &app, [&]() { ++removed; });
            QObject::connect(group1, &QObject::destroyed, &app, [&]() { ++removed; });
            QObject::connect(group2, &QObject::destroyed, &app, [&]() { ++removed; });
            root.panes = {"a", "c"};
            root.sync();
            settle();
            root.checkOrder();
            require(removed == 3, "removed physical/virtual panes were not deleted exactly once");
            root.panes.clear();
            root.sync();
            settle();
            root.checkOrder();
            root.panes = {"a"};
            root.vertical = false;
            root.sync();
            settle();
            root.checkOrder();
            require(root.native() == splitter && splitter->orientation() == Qt::Horizontal,
                    "empty/repopulate lost the splitter or failed to reset its orientation");
            root.show = false;
            root.sync();
            settle();
            require(destroyed == 1, "splitter removal did not delete it exactly once");
            root.show = true;
            root.sync();
            root.checkOrder();
            QObject::connect(root.native(), &QObject::destroyed, &app, [&]() { ++destroyed; });
        }
        require(destroyed == 2, "root teardown did not release its splitter");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

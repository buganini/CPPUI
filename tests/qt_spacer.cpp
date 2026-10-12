#include "cppui/qt/layout.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/state.hpp"
#include <QApplication>
#include <QEvent>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    int declarations = 0, items = 0;
    class Item: public QSpacerItem {
        public:
        explicit Item(QSizePolicy policy): QSpacerItem(0, 0, policy.horizontalPolicy(), policy.verticalPolicy()) { ++items; }
        ~Item() override { --items; }
    };
    class Spacer: public CPPUI::Qt::Spacer {
        public:
        Spacer() { ++declarations; }
        ~Spacer() override { --declarations; }
        void update(CPPUI::Node * prev) override {
            CPPUI::Qt::Spacer::update(prev);
            if(!prev) {
                auto replacement = new Item(ui->sizePolicy());
                delete ui;
                ui = replacement;
            }
        }
    };
    CPPUI::Node * find(CPPUI::Node & node, const char * id) {
        if(node.has_id && node._id == id) return &node;
        for(auto child: *node.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    class VirtualSpacer: public CPPUI::Qt::QtView {
        CPPUI::State<int> & count;
        public:
        explicit VirtualSpacer(CPPUI::State<int> & count): count(count) {}
        void content() override { for(int i = 0; i < count; ++i) (new Spacer())->id("gap"); }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        CPPUI::State<int> count{1};
        bool show = true, reverse = false;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) if(show) {
                for(auto row: CPPUI::HBox().id("row")) {
                    if(reverse) CPPUI::Label("end").id("end");
                    CPPUI::Label("start").id("start");
                    (new VirtualSpacer(count))->id("virtual");
                    if(!reverse) CPPUI::Label("end").id("end");
                }
                (new Spacer())->id("vertical");
            }
        }
        QWidget * window() { return static_cast<QWidget *>(children->front()->outer()); }
        QSpacerItem * gap() { return static_cast<QSpacerItem *>(find(*this, "gap")->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        {
            Root root;
            root.sync();
            root.window()->resize(600, 400);
            root.window()->show();
            settle();
            auto gap = root.gap();
            auto row = static_cast<CPPUI::Qt::BaseLayout *>(find(root, "row"));
            auto vertical = static_cast<QSpacerItem *>(find(root, "vertical")->outer());
            require(gap->geometry().width() > 300 && vertical->geometry().height() > 250, "spacers did not expand along their box axes");
            require(gap->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding
                && gap->sizePolicy().verticalPolicy() == QSizePolicy::Preferred, "row spacer policy differs from PUI");
            require(vertical->sizePolicy().horizontalPolicy() == QSizePolicy::Preferred
                && vertical->sizePolicy().verticalPolicy() == QSizePolicy::Expanding, "column spacer policy differs from PUI");
            require(!find(root, "gap")->is_movable && find(root, "gap")->is_terminal, "spacer declaration flags differ from PUI");
            for(int i = 0; i < 20; ++i) { root.sync(); settle(); require(root.gap() == gap && declarations == 2 && items == 2, "redraw leaked/replaced spacers"); }
            root.count = 2;
            settle();
            row = static_cast<CPPUI::Qt::BaseLayout *>(find(root, "row"));
            require(items == 3 && row->layout->count() == 4
                && std::abs(row->layout->itemAt(1)->geometry().width() - row->layout->itemAt(2)->geometry().width()) <= 1,
                "multiple virtual spacers did not share spare width");
            root.count = 1;
            root.reverse = true;
            root.sync();
            settle();
            require(root.gap() == gap && items == 2, "neighboring keyed move replaced the fixed spacer");
            root.count = 0;
            settle();
            require(items == 1, "independent virtual removal leaked spacer item");
            root.count = 1;
            settle();
            for(int i = 0; i < 10; ++i) {
                root.show = false; root.sync(); settle();
                require(items == 0 && declarations == 0, "parent removal leaked declarations/items");
                root.show = true; root.sync(); settle();
            }
            auto declaration = static_cast<CPPUI::Qt::Spacer *>(find(root, "gap"));
            delete root.window();
            require(!declaration->ui && items == 0, "external layout deletion left stale spacer items");
        }
        settle();
        require(items == 0 && declarations == 0, "root teardown leaked spacers");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

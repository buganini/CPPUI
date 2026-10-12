#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/label.hpp"
#include <QPointer>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QCoreApplication::processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    int spacers = 0;
    class Item: public QSpacerItem {
        public:
        Item(): QSpacerItem(0, 0) { ++spacers; }
        ~Item() override { --spacers; }
    };
    class Spacer: public CPPUI::Qt::Spacer {
        public:
        void update(CPPUI::Node * previous) override {
            CPPUI::Qt::Spacer::update(previous);
            if(!previous) { delete ui; ui = new Item(); }
        }
    };
    CPPUI::Node & cell(CPPUI::Node & node, int row, int column, int rows = 1, int columns = 1) {
        node.grid_row = row; node.grid_column = column;
        node.grid_rowspan = rows; node.grid_columnspan = columns;
        return node;
    }
    CPPUI::Node * find(CPPUI::Node & root, const char * id) {
        if(root.has_id && root._id == id) return &root;
        for(auto child: *root.children) if(auto result = find(*child, id)) return result;
        return nullptr;
    }
    class Root: public CPPUI::Application {
        public:
        bool show = true, reverse = false;
        int row = 1;
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window().size(600, 400)) for(auto column: CPPUI::VBox()) if(show) {
                for(auto grid: CPPUI::Grid().id("grid")) {
                    cell(CPPUI::Label("span").id("span"), 0, 1, 2, 2);
                    cell(CPPUI::Label("moving").id("moving"), row, 0, 0, 0);
                    cell(CPPUI::Label("first").id("first"), 0, 0);
                    CPPUI::Label("unplaced").id("unplaced");
                    cell(*new Spacer(), 9, 9);
                }
                for(auto stack: CPPUI::Stack().id("stack")) {
                    if(reverse) CPPUI::Label("Top").id("top");
                    for(auto bottom: CPPUI::HBox().id("bottom")) CPPUI::Label("Bottom");
                    if(!reverse) CPPUI::Label("Top").id("top");
                    new Spacer();
                }
            }
        }
        QWidget * widget(const char * id) { return static_cast<QWidget *>(find(*this, id)->outer()); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync(); settle();
            auto grid = static_cast<CPPUI::Qt::Grid *>(find(root, "grid"));
            auto stack = static_cast<CPPUI::Qt::Stack *>(find(root, "stack"));
            auto nativeGrid = grid->qtlayout;
            auto nativeStack = stack->qtlayout;
            auto span = root.widget("span"), top = root.widget("top"), bottom = root.widget("bottom");
            int row, column, rows, columns;
            nativeGrid->getItemPosition(nativeGrid->indexOf(span), &row, &column, &rows, &columns);
            require(row == 0 && column == 1 && rows == 2 && columns == 2, "grid span/placement was not applied");
            require(nativeGrid->count() == 3 && !find(root, "unplaced"), "unplaced or ignored grid children were mounted");
            nativeGrid->getItemPosition(nativeGrid->indexOf(root.widget("moving")), &row, &column, &rows, &columns);
            require(rows == 1 && columns == 1, "zero spans did not use the reference default");
            require(nativeStack->count() == 2 && nativeStack->stackingMode() == QStackedLayout::StackAll
                && nativeStack->widget(0) == top && nativeStack->currentWidget() == top, "Stack initial reversed order or overlap mode is wrong");
            require(top->isVisible() && bottom->isVisible() && top->geometry() == bottom->geometry(), "Stack children do not overlap");
            require(grid->ui->contentsMargins() == QMargins() && stack->ui->contentsMargins() == QMargins()
                && bottom->contentsMargins() == QMargins(), "nested layout padding was added twice");
            for(int i = 0; i < 12; ++i) { root.sync(); settle(); }
            require(root.widget("span") == span && nativeStack->currentWidget() == top && spacers == 2, "redraw replaced children or leaked ignored spacers");
            root.reverse = true; root.sync(); settle();
            require(nativeStack->widget(0) == bottom && nativeStack->widget(1) == top, "keyed Stack order was not reversed");
            // Removing/reinserting the current layer lets Qt raise its neighbor.
            require(nativeStack->currentWidget() == bottom, "Stack did not follow Qt's native current-layer transition");
            QPointer<QWidget> oldCell = root.widget("moving");
            root.row = 2; root.sync(); settle();
            require(oldCell.isNull() && nativeGrid->itemAtPosition(2, 0)->widget() == root.widget("moving"), "changed grid identity did not replace its cell");
            for(int i = 0; i < 12; ++i) {
                root.show = false; root.sync(); settle(); require(spacers == 0, "indirect teardown leaked ignored QSpacerItems");
                root.show = true; root.sync(); settle();
            }
            grid = static_cast<CPPUI::Qt::Grid *>(find(root, "grid"));
            delete grid->ui; require(!grid->ui && !grid->qtlayout, "external grid deletion retained native access");
        }
        settle();
        require(spacers == 0 && qt.topLevelWidgets().isEmpty(), "container teardown retained native resources");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

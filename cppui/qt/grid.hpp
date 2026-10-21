#ifndef CPPUI_QT_GRID
#define CPPUI_QT_GRID

#include "base.hpp"
#include "../grid.hpp"
#include <QGridLayout>

namespace CPPUI { namespace Qt {
    class Grid: public ::CPPUI::detail::GridLayout<LayoutWidget> {
        protected:
        void clearNative() override { qtlayout = nullptr; gridChildren.clear(); BaseWidget::clearNative(); }
        public:
        QGridLayout * qtlayout = nullptr;
        Grid() { is_grid_layout = true; }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Grid *>(previous);
            retainGrid(prev && prev->ui ? prev : nullptr);
            if(prev && prev->ui) { ui = prev->ui; qtlayout = prev->qtlayout; }
            else { ui = new Widget<QWidget>(); qtlayout = new QGridLayout(ui); }
            qtlayout->setContentsMargins(0, 0, 0, 0);
            applyPadding();
        }
        void * inner() override { return qtlayout; }
        void addChild(int index, Node * child) override {
            if(child->is_outoforder) return;
            mountGrid(index,child);
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) widget->ui->setParent(ui);
        }
        void removeChild(int index, Node * child) override {
            if(child->is_outoforder) return;
            unmountGrid(index,child);
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) qtlayout->removeWidget(widget->ui);
        }
        void postSync() override {
            if(!qtlayout) return;
            auto placed=cells();
            while(auto item=qtlayout->takeAt(0)) delete item;
            for(int column=0;column<std::max(qtlayout->columnCount(),grid_columns);++column)
                qtlayout->setColumnStretch(column,column<grid_columns ? 1 : 0);
            for(const auto & item:placed) {
                auto widget=dynamic_cast<BaseWidget *>(item.node);
                if(!widget || !widget->ui) continue;
                const auto & cell=item.cell;
                qtlayout->addWidget(widget->ui,cell.row,cell.column,cell.rowspan,cell.columnspan);
            }
        }
    };
}}
namespace CPPUI { inline Qt::Grid & Grid() { return *new Qt::Grid(); } }
#endif

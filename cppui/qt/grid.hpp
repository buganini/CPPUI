#ifndef CPPUI_QT_GRID
#define CPPUI_QT_GRID

#include "base.hpp"
#include "../grid.hpp"
#include "../gap.hpp"
#include <QGridLayout>

namespace CPPUI { namespace Qt {
    class Grid: public ::CPPUI::detail::GridLayout<LayoutWidget> {
        protected:
        void clearNative() override { qtlayout = nullptr; gridChildren.clear(); BaseWidget::clearNative(); }
        public:
        QGridLayout * qtlayout = nullptr;
        Gap spacing;
        explicit Grid(Gap gap=10):spacing(gap) { is_grid_layout = true; expandX=expandY=1; }
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
            if(auto spacer=dynamic_cast<Spacer *>(child)) if(spacer->ui) qtlayout->removeItem(spacer->ui);
        }
        void postSync() override {
            if(!qtlayout) return;
            qtlayout->setVerticalSpacing(spacing.rows); qtlayout->setHorizontalSpacing(spacing.columns);
            auto placed=cells();
            const auto columns=::CPPUI::detail::nativeStretches(::CPPUI::detail::trackStretches(placed,true,this));
            const auto rows=::CPPUI::detail::nativeStretches(::CPPUI::detail::trackStretches(placed,false,this));
            while(auto item=qtlayout->takeAt(0)) if(!item->spacerItem()) delete item;
            for(int column=0;column<std::max(qtlayout->columnCount(),int(columns.size()));++column)
                qtlayout->setColumnStretch(column,column<int(columns.size()) ? columns[column] : 0);
            for(int row=0;row<std::max(qtlayout->rowCount(),int(rows.size()));++row)
                qtlayout->setRowStretch(row,row<int(rows.size()) ? rows[row] : 0);
            const bool x=std::any_of(columns.begin(),columns.end(),[](int n){return n>0;});
            const bool y=std::any_of(rows.begin(),rows.end(),[](int n){return n>0;});
            qtlayout->setAlignment((x ? ::Qt::Alignment() : ::Qt::AlignLeft) | (y ? ::Qt::Alignment() : ::Qt::AlignTop));
            for(const auto & item:placed) {
                const auto & cell=item.cell;
                if(auto spacer=dynamic_cast<Spacer *>(item.node)) {
                    if(spacer->ui) qtlayout->addItem(spacer->ui,cell.row,cell.column,cell.rowspan,cell.columnspan);
                } else if(auto widget=dynamic_cast<BaseWidget *>(item.node)) {
                    if(!widget->ui) continue;
                    widget->applyGridPolicy();
                    const auto alignment=(widget->expansionPriority(true,this)>0 ? ::Qt::Alignment() : ::Qt::AlignLeft)
                        | (widget->expansionPriority(false,this)>0 ? ::Qt::Alignment() : ::Qt::AlignVCenter);
                    qtlayout->addWidget(widget->ui,cell.row,cell.column,cell.rowspan,cell.columnspan,alignment);
                }
            }
        }
    };
}}
namespace CPPUI { inline Qt::Grid & Grid(Gap gap=10) { return *new Qt::Grid(gap); } }
#endif

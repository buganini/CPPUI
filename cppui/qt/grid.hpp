#ifndef CPPUI_QT_GRID
#define CPPUI_QT_GRID

#include "base.hpp"
#include <QGridLayout>

namespace CPPUI { namespace Qt {
    class Grid: public LayoutWidget {
        protected:
        void clearNative() override { qtlayout = nullptr; BaseWidget::clearNative(); }
        public:
        QGridLayout * qtlayout = nullptr;
        Grid() { is_grid_layout = true; }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Grid *>(previous);
            if(prev && prev->ui) { ui = prev->ui; qtlayout = prev->qtlayout; }
            else { ui = new Widget<QWidget>(); qtlayout = new QGridLayout(ui); }
            qtlayout->setContentsMargins(0, 0, 0, 0);
            applyPadding();
        }
        void * inner() override { return qtlayout; }
        void addChild(int, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) qtlayout->addWidget(widget->ui, child->grid_row, child->grid_column,
                std::max(1, child->grid_rowspan), std::max(1, child->grid_columnspan));
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(widget && widget->ui) widget->ui->setParent(nullptr);
        }
    };
}}
namespace CPPUI { inline Qt::Grid & Grid() { return *new Qt::Grid(); } }
#endif

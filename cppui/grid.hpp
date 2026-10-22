#ifndef CPPUI_GRID
#define CPPUI_GRID

#include "view.hpp"
#include <cstdint>
#include <limits>

namespace CPPUI { namespace detail {
    struct GridItem {
        Node * node;
        GridCell cell;
    };

    inline void expandGridCells(std::vector<GridItem> & items,int columns) {
        int bottom=0;
        for(const auto & item:items) bottom=std::max(bottom,item.cell.row+item.cell.rowspan);
        for(auto & item:items) {
            auto & cell=item.cell;
            // Jump to the nearest blocking rectangle, keeping work independent
            // of span sizes. Earlier children's expansions already occupy space.
            int right=columns;
            for(const auto & other:items) {
                const auto & occupied=other.cell;
                if(occupied.column>=cell.column+cell.columnspan
                   && occupied.row<cell.row+cell.rowspan && occupied.row+occupied.rowspan>cell.row)
                    right=std::min(right,occupied.column);
            }
            cell.columnspan=right-cell.column;
            int end=bottom;
            for(const auto & other:items) {
                const auto & occupied=other.cell;
                if(occupied.row>=cell.row+cell.rowspan
                   && occupied.column<cell.column+cell.columnspan && occupied.column+occupied.columnspan>cell.column)
                    end=std::min(end,occupied.row);
            }
            cell.rowspan=end-cell.row;
            item.node->grid_cell=cell;
        }
    }

    // Store occupied rectangles rather than allocating one entry per spanned
    // cell. A collision skips to that rectangle's right edge.
    inline std::vector<GridItem> gridCells(const std::vector<Node *> & children,
                                         int columns, Optional<bool> dense={},bool expandChildren=true) {
        std::vector<GridItem> result;
        result.reserve(children.size());
        int64_t cursor=0, previousRow=0;
        for(auto child:children) {
            child->grid_cell.reset();
            if(child->is_outoforder) continue;
            if(!columns) {
                if(child->grid_row>=0 && child->grid_column>=0) {
                    GridCell cell(child->grid_row,child->grid_column,
                                  std::max(1,child->grid_rowspan),std::max(1,child->grid_columnspan));
                    child->grid_cell=cell;result.push_back({child,cell});
                }
                continue;
            }
            if(!child->is_visible) continue;
            const int rows=child->grid_rowspan==-1 ? 1 : child->grid_rowspan;
            const int declaredColumns=child->grid_columnspan==-1 ? 1 : child->grid_columnspan;
            if(rows<1 || declaredColumns<1) throw std::invalid_argument("Automatic Grid spans must be positive");
            const int span=std::min(columns,declaredColumns);
            int64_t position=dense ? (*dense ? 0 : cursor) : previousRow*columns;
            GridCell cell;
            while(true) {
                const int64_t row=position/columns,column=position%columns;
                if(row>std::numeric_limits<int>::max()-rows)
                    throw std::out_of_range("Grid placement exceeds the supported integer range");
                if(column+span>columns) { position=(row+1)*columns;continue; }
                int64_t next=position;
                for(const auto & item:result) {
                    const auto & occupied=item.cell;
                    if(row< int64_t(occupied.row)+occupied.rowspan && row+rows>occupied.row
                       && column< int64_t(occupied.column)+occupied.columnspan && column+span>occupied.column)
                        next=std::max(next,row*columns+occupied.column+occupied.columnspan);
                }
                if(next!=position) {position=next;continue;}
                cell=GridCell(static_cast<int>(row),static_cast<int>(column),rows,span);break;
            }
            cursor=position+span;previousRow=cell.row;
            child->grid_cell=cell;result.push_back({child,cell});
        }
        if(columns && expandChildren) expandGridCells(result,columns);
        return result;
    }

    template<class Base>
    class GridLayout: public Base {
    protected:
        std::vector<Node *> gridChildren;
        Optional<bool> gridDense;
        bool gridExpandChildren=true;
        void retainGrid(GridLayout * previous) {
            if(previous) gridChildren=std::move(previous->gridChildren);
        }
        void mountGrid(int index,Node * child) {
            if(!child->is_outoforder) gridChildren.insert(gridChildren.begin()+index,child);
        }
        void unmountGrid(int index,Node * child) {
            if(!child->is_outoforder) gridChildren.erase(gridChildren.begin()+index);
        }
        std::vector<GridItem> cells() { return gridCells(gridChildren,this->grid_columns,gridDense,gridExpandChildren); }
        static void validateColumns(int count) {
            if(count<1) throw std::invalid_argument("Grid columns must be positive");
        }
    public:
        GridLayout() { this->is_grid_layout=true; }
        GridLayout & columns(int count,grid::Dense dense={},grid::ExpandChildren expand={}) {
            validateColumns(count);this->grid_columns=count;gridDense=dense.value;
            gridExpandChildren=expand.value.valueOr(true);return *this;
        }
        GridLayout & columns(int count,grid::ExpandChildren expand,grid::Dense dense={}) {
            return columns(count,dense,expand);
        }
        GridLayout & wideColumns(Breakpoint minWidth,int count,grid::ExpandChildren expand={}) {
            validateColumns(count);
            if(this->wideMatches(minWidth,WideMethod::Columns)) {
                this->grid_columns=count;
                if(expand.value) gridExpandChildren=*expand.value;
            }
            return *this;
        }
        void rebindChild(Node * previous,Node * current) override {
            Base::rebindChild(previous,current);
            for(auto & child:gridChildren) if(child==previous) child=current;
        }
    };
}}
#endif

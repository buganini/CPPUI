#include "cppui/grid.hpp"
#include <iostream>
#include <random>
#include <set>

static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
struct Item:CPPUI::Node {void destroy(bool) override {}};
struct Grid:CPPUI::detail::GridLayout<Item> {
    using GridLayout::cells;
    using GridLayout::gridChildren;
};
static std::vector<CPPUI::GridCell> reference(const std::vector<CPPUI::Node *> & items,int columns,CPPUI::Optional<bool> dense,bool expand=false) {
    std::set<std::pair<int,int>> occupied;
    std::vector<CPPUI::GridCell> result;
    int cursor=0,previousRow=0;
    for(auto item:items) {
        if(!item->is_visible || item->is_outoforder) continue;
        const int rows=item->grid_rowspan,cols=std::min(columns,item->grid_columnspan);
        int position=dense ? (*dense ? 0 : cursor) : previousRow*columns;
        for(;;++position) {
            int row=position/columns,col=position%columns;
            bool free=col+cols<=columns;
            for(int r=row;free && r<row+rows;++r) for(int c=col;c<col+cols;++c)
                if(occupied.count({r,c})) free=false;
            if(!free) continue;
            for(int r=row;r<row+rows;++r) for(int c=col;c<col+cols;++c) occupied.insert({r,c});
            result.emplace_back(row,col,rows,cols);cursor=position+cols;previousRow=row;break;
        }
    }
    if(expand) {
        int bottom=0;
        for(const auto & cell:result) bottom=std::max(bottom,cell.row+cell.rowspan);
        for(auto & cell:result) {
            while(cell.column+cell.columnspan<columns) {
                bool free=true;
                for(int r=cell.row;r<cell.row+cell.rowspan;++r)
                    if(occupied.count({r,cell.column+cell.columnspan})) free=false;
                if(!free) break;
                for(int r=cell.row;r<cell.row+cell.rowspan;++r) occupied.insert({r,cell.column+cell.columnspan});
                ++cell.columnspan;
            }
            while(cell.row+cell.rowspan<bottom) {
                bool free=true;
                for(int c=cell.column;c<cell.column+cell.columnspan;++c)
                    if(occupied.count({cell.row+cell.rowspan,c})) free=false;
                if(!free) break;
                for(int c=cell.column;c<cell.column+cell.columnspan;++c) occupied.insert({cell.row+cell.rowspan,c});
                ++cell.rowspan;
            }
        }
    }
    return result;
}
int main() {
    try {
        Item owner;owner.update(nullptr);owner.size();
        Grid grid;grid.columns(4,CPPUI::grid::dense(true));grid.columns(3);
        for(int value:{0,-1}) {bool threw=false;try {grid.columns(value);}catch(const std::invalid_argument &) {threw=true;} require(threw,"invalid column count accepted");}
        Item a,b,c; a.grid(CPPUI::grid::columnspan(2));b.grid(CPPUI::grid::columnspan(2));
        std::vector<CPPUI::Node *> items{&a,&b,&c};
        auto dense=CPPUI::detail::gridCells(items,3,true,false);
        require(dense.back().cell==CPPUI::GridCell(0,2),"dense mode failed to backfill");
        require(a.grid_row==-1 && a.grid_column==-1,"computed position changed declaration coordinates");
        require(CPPUI::detail::gridCells(items,3,false,false).back().cell==CPPUI::GridCell(1,2),"forward placement failed");
        a.is_visible=false;CPPUI::detail::gridCells(items,1);
        require(!a.grid_cell && b.grid_cell->columnspan==1 && b.grid_columnspan==2,"hidden/clamped span changed declaration");
        CPPUI::detail::gridCells(items,3,{},false);require(b.grid_cell->columnspan==2,"wider span was not restored");
        bool threw=false;try {b.grid(CPPUI::grid::row(1),CPPUI::grid::rowspan(0));}catch(const std::invalid_argument &) {threw=true;}
        require(threw && b.grid_row==-1,"invalid named grid call partially changed fields");
        a.is_visible=true;a.grid_rowspan=0;threw=false;
        try {CPPUI::detail::gridCells(items,3);}catch(const std::invalid_argument &) {threw=true;}
        require(threw,"automatic grid accepted zero span");
        Item base,responsive;
        base.grid(CPPUI::grid::row(0),CPPUI::grid::column(0));
        responsive.grid(CPPUI::grid::row(0),CPPUI::grid::column(0)).wideGrid(600,CPPUI::grid::row(2));
        responsive.grid_row=2; // Simulate a matching responsive override without native geometry.
        require(base==responsive,"responsive coordinates changed base identity");
        responsive.grid(CPPUI::grid::columnspan(2));
        require(!(base==responsive),"later base grid call did not update identity");
        base.grid(CPPUI::grid::columnspan(2));require(base==responsive,"base grid merged responsive coordinates into identity");
        Item simple[4];grid.gridChildren={&simple[0],&simple[1],&simple[2],&simple[3]};
        grid.columns(3);
        const std::vector<CPPUI::GridCell> expanded={{0,0,1,1},{0,1,2,1},{0,2,2,1},{1,0,1,1}};
        for(bool expand:{true,false,true}) {
            grid.columns(3,CPPUI::grid::expandChildren(expand));
            auto cells=grid.cells();
            for(std::size_t i=0;i<cells.size();++i) {
                require(cells[i].cell==(expand ? expanded[i] : CPPUI::GridCell(i/3,i%3)),"expansion toggle differs from PUI");
                require(simple[i].grid_rowspan==-1 && simple[i].grid_columnspan==-1,"expansion changed declared spans");
            }
        }
        grid.columns(3,CPPUI::grid::expandChildren(false),CPPUI::grid::dense(true));
        require(grid.cells()[1].cell.rowspan==1,"named expansion/dense order failed");
        grid.columns(3);require(grid.cells()[1].cell.rowspan==2,"columns did not reset default expansion");
        grid.parent=&owner;
        for(int width:{599,600,800,900,400}) {
            owner.sizeState->publish(width,400);
            grid.columns(3,CPPUI::grid::expandChildren(false))
                .wideColumns(500,3)
                .wideColumns(600,3,CPPUI::grid::expandChildren(true))
                .wideColumns(800,3)
                .wideColumns(900,3,CPPUI::grid::expandChildren(false));
            require(grid.cells()[1].cell.rowspan==(width>=600 && width<900 ? 2 : 1),"breakpoint expansion inheritance/override differs");
        }
        Item left,right;left.grid(CPPUI::grid::columnspan(2));right.grid(CPPUI::grid::columnspan(3));
        auto edge=CPPUI::detail::gridCells({&left,&right},4);
        require(edge[0].cell==CPPUI::GridCell(0,0,1,4) && edge[1].cell==CPPUI::GridCell(1,0,1,4),"right expansion did not precede downward expansion");
        left.grid(CPPUI::grid::row(0),CPPUI::grid::column(0));right.grid(CPPUI::grid::row(1),CPPUI::grid::column(2));
        auto manual=CPPUI::detail::gridCells({&left,&right},0);
        require(manual[0].cell==CPPUI::GridCell(0,0,1,2) && manual[1].cell==CPPUI::GridCell(1,2,1,3),"manual grid was expanded");
        require(CPPUI::detail::gridCells({},3).empty(),"empty expansion produced cells");
        left.is_visible=false;require(CPPUI::detail::gridCells({&left},3).empty() && !left.grid_cell,"hidden expansion produced cells");
        left.is_visible=true;left.is_outoforder=true;
        require(CPPUI::detail::gridCells({&left},3).empty(),"out-of-order item occupied expansion space");
        Item huge,small;huge.grid_rowspan=std::numeric_limits<int>::max();
        auto large=CPPUI::detail::gridCells({&huge,&small},std::numeric_limits<int>::max());
        require(large[1].cell.rowspan==std::numeric_limits<int>::max() && large[1].cell.columnspan==std::numeric_limits<int>::max()-1,
            "large rectangle expansion failed");
        for(int width:{599,600,800,400,600}) {
            owner.sizeState->publish(width,400);
            Item field;field.parent=&owner;
            field.grid(CPPUI::grid::row(0),CPPUI::grid::column(0))
                .wideGrid(600,CPPUI::grid::row(1),CPPUI::grid::column(1),CPPUI::grid::columnspan(2))
                .wideGrid(800,CPPUI::grid::rowspan(2)).wideGrid(600,CPPUI::grid::column(0)).id("field").ref("field");
            require(field.grid_row==(width>=600 ? 1 : 0) && field.grid_column==0
                && field.grid_columnspan==(width>=600 ? 2 : -1) && field.grid_rowspan==(width>=800 ? 2 : -1),
                "wideGrid boundary or override order differs");
            require(*field.grid_identity==CPPUI::GridCell(0,0,-1,-1),"wideGrid/id/ref changed the base cell");
        }
        Item tiles[7];items.clear();
        const int spans[7][2]={{2,1},{1,1},{3,2},{3,1},{1,2},{1,1},{2,2}};
        for(int i=0;i<7;++i) {tiles[i].grid_rowspan=spans[i][0];tiles[i].grid_columnspan=spans[i][1];items.push_back(&tiles[i]);}
        require(CPPUI::detail::gridCells(items,4)[5].cell.row==3,"ordered waterfall row differs");
        require(CPPUI::detail::gridCells(items,4,true)[5].cell.row==2,"dense waterfall row differs");
        require(CPPUI::detail::gridCells(items,4,false)[5].cell.row==4,"forward waterfall row differs");
        std::mt19937 random(42);
        for(int run=0;run<300;++run) {
            int columns=1+random()%6;
            for(auto item:items) {item->grid_rowspan=1+random()%4;item->grid_columnspan=1+random()%5;item->is_visible=random()%4!=0;}
            for(auto mode:{CPPUI::Optional<bool>(),CPPUI::Optional<bool>(true),CPPUI::Optional<bool>(false)}) {
                for(bool expand:{false,true}) {
                    auto expected=reference(items,columns,mode,expand);auto actual=CPPUI::detail::gridCells(items,columns,mode,expand);
                    require(expected.size()==actual.size(),"placement item count differs");
                    for(std::size_t i=0;i<expected.size();++i) require(expected[i]==actual[i].cell,"rectangle placement differs from cell reference");
                }
            }
        }
        std::cout<<"grid placement passed\n";
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

#include "cppui/grid.hpp"
#include <iostream>
#include <random>
#include <set>

static void require(bool value,const char * message) {if(!value) throw std::runtime_error(message);}
struct Item:CPPUI::Node {void destroy(bool) override {}};
struct Grid:CPPUI::detail::GridLayout<Item> {};
static std::vector<CPPUI::GridCell> reference(const std::vector<CPPUI::Node *> & items,int columns,CPPUI::Optional<bool> dense) {
    std::set<std::pair<int,int>> occupied;
    std::vector<CPPUI::GridCell> result;
    int cursor=0,previousRow=0;
    for(auto item:items) {
        if(!item->is_visible) continue;
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
    return result;
}
int main() {
    try {
        Grid grid;grid.columns(4,CPPUI::grid::dense(true));grid.columns(3);
        for(int value:{0,-1}) {bool threw=false;try {grid.columns(value);}catch(const std::invalid_argument &) {threw=true;} require(threw,"invalid column count accepted");}
        Item a,b,c; a.grid(CPPUI::grid::columnspan(2));b.grid(CPPUI::grid::columnspan(2));
        std::vector<CPPUI::Node *> items{&a,&b,&c};
        auto dense=CPPUI::detail::gridCells(items,3,true);
        require(dense.back().cell==CPPUI::GridCell(0,2),"dense mode failed to backfill");
        require(a.grid_row==-1 && a.grid_column==-1,"computed position changed declaration coordinates");
        require(CPPUI::detail::gridCells(items,3,false).back().cell==CPPUI::GridCell(1,2),"forward placement failed");
        a.is_visible=false;CPPUI::detail::gridCells(items,1);
        require(!a.grid_cell && b.grid_cell->columnspan==1 && b.grid_columnspan==2,"hidden/clamped span changed declaration");
        CPPUI::detail::gridCells(items,3);require(b.grid_cell->columnspan==2,"wider span was not restored");
        bool threw=false;try {b.grid(CPPUI::grid::row(1),CPPUI::grid::rowspan(0));}catch(const std::invalid_argument &) {threw=true;}
        require(threw && b.grid_row==-1,"invalid named grid call partially changed fields");
        a.is_visible=true;a.grid_rowspan=0;threw=false;
        try {CPPUI::detail::gridCells(items,3);}catch(const std::invalid_argument &) {threw=true;}
        require(threw,"automatic grid accepted zero span");
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
                auto expected=reference(items,columns,mode);auto actual=CPPUI::detail::gridCells(items,columns,mode);
                require(expected.size()==actual.size(),"placement item count differs");
                for(std::size_t i=0;i<expected.size();++i) require(expected[i]==actual[i].cell,"rectangle placement differs from cell reference");
            }
        }
        std::cout<<"grid placement passed\n";
    } catch(const std::exception & error) {std::cerr<<error.what()<<'\n';return 1;}
}

#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void grid(Model &) {
        CPPUI::Label("Manual Grid: tile coordinates and spans are explicit.");
        for(auto scope:CPPUI::Grid().id("example-grid").layout(CPPUI::layout::weight(1))) {
            struct Tile {const char * name;int row,column,rows,columns,background,foreground;};
            const Tile tiles[]={
                {"A",0,0,2,1,0x244B5A,0xFFF7E6},
                {"B",0,1,1,2,0xA84A3A,0xFFF7E6},
                {"C",1,1,1,1,0xD97B35,0x1C2A30},
                {"D",1,2,2,1,0xF2E6CF,0x1C2A30},
                {"E",2,0,1,2,0x8FA38A,0x1C2A30},
            };
            for(const auto & tile:tiles)
                CPPUI::Label(std::string(tile.name)+"\nrow="+std::to_string(tile.row)+", column="+std::to_string(tile.column)
                    +"\nspan="+std::to_string(tile.rows)+" × "+std::to_string(tile.columns)).id(tile.name)
                    .grid(CPPUI::grid::row(tile.row),CPPUI::grid::column(tile.column),
                          CPPUI::grid::rowspan(tile.rows),CPPUI::grid::columnspan(tile.columns))
                    .style(CPPUI::style::bgColor(tile.background),CPPUI::style::color(tile.foreground))
                    .layout(CPPUI::layout::padding(8),CPPUI::layout::height(tile.rows*60));
        }
    }
}

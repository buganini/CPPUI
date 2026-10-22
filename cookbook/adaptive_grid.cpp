#include "cppui/backend.hpp"
#include "recipes.hpp"
#include <algorithm>

namespace Cookbook {
    void adaptiveGrid(Model & model) {
        // Declare each wide* method's breakpoints from small to large; later matches win.
        for(auto body:CPPUI::VBox().layout(CPPUI::layout::weight(1))) {
            CPPUI::Label("Adaptive Grid: 1–4 columns (labels: rows×cols).\nFour columns from 768 px.");
            CPPUI::Label("Watch blue F (original order; C shown):\nOrdered → row 4 | Dense → row 3 | Forward → row 5");
            for(auto controls:CPPUI::HVBox(650)) {
                CPPUI::RadioButton("Ordered",std::string("ordered"),model.gridMode);
                CPPUI::RadioButton("Dense",std::string("dense"),model.gridMode);
                CPPUI::RadioButton("Forward",std::string("forward"),model.gridMode);
                CPPUI::Checkbox("Show C",model.gridShowC).id("grid-show-c");
                CPPUI::Checkbox("Expand children",model.gridExpand).id("grid-expand");
                CPPUI::Checkbox("Reverse tiles",model.gridReverse).id("grid-reverse");
            }
            const std::string mode=model.gridMode;
            CPPUI::Optional<bool> dense;
            if(mode!="ordered") dense=mode=="dense";
            for(auto scroll:CPPUI::Scroll().layout(CPPUI::layout::weight(1))) {
                for(auto scope:CPPUI::Grid().columns(1,CPPUI::grid::dense(dense),CPPUI::grid::expandChildren(model.gridExpand))
                    .wideColumns(384,2)
                    .wideColumns(576,3)
                    .wideColumns(768,4).id("adaptive-grid")) {
                    struct Tile {const char * name;int rows,columns,background,foreground;};
                    std::vector<Tile> tiles={
                        {"A",2,1,0x244B5A,0xFFF7E6},{"B",1,1,0xA84A3A,0xFFF7E6},
                        {"C",3,2,0xD97B35,0x1C2A30},{"D",3,1,0xF2E6CF,0x1C2A30},
                        {"E",1,2,0x8FA38A,0x1C2A30},{"F",1,1,0x366C91,0xFFF7E6},
                        {"G",2,2,0xD3B98B,0x1C2A30},
                    };
                    if(model.gridReverse) std::reverse(tiles.begin(),tiles.end());
                    for(const auto & tile:tiles)
                        CPPUI::Label(std::string(tile.name)+"\n"+std::to_string(tile.rows)+"×"+std::to_string(tile.columns)).id(tile.name)
                            .grid(CPPUI::grid::rowspan(tile.rows),CPPUI::grid::columnspan(tile.columns))
                            .visible(std::string(tile.name)!="C" || bool(model.gridShowC))
                            .style(CPPUI::style::bgColor(tile.background),CPPUI::style::color(tile.foreground))
                            .layout(CPPUI::layout::height(tile.rows*60),CPPUI::layout::padding(6));
                    CPPUI::TextField(model.gridText).id("grid-editor").grid(CPPUI::grid::columnspan(1))
                        .wideGrid(576,CPPUI::grid::columnspan(2));
                }
            }
        }
    }
}

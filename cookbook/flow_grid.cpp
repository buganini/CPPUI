#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void flowGrid(Model & model) {
        bool expandChildren = model.flowGridExpand;
        for(auto body:CPPUI::VBox().layout(CPPUI::layout::weight(1))) {
            CPPUI::Label("Flow Grid: items auto-flow into columns.");
            CPPUI::Checkbox("expandChildren", model.flowGridExpand);
            for(auto scroll:CPPUI::Scroll().layout(CPPUI::layout::weight(1))) {
                for(auto scope:CPPUI::Grid(8).columns(3, CPPUI::grid::expandChildren(expandChildren))
                    .wideColumns(500, 5, CPPUI::grid::expandChildren(expandChildren))) {
                    for(auto box:CPPUI::VBox()
                        .style(CPPUI::style::bgColor(0x4A90D9))
                        .layout(CPPUI::layout::padding(8), CPPUI::layout::minHeight(30))
                        .grid(CPPUI::grid::columnspan(5))) {
                        CPPUI::Label("Header").style(CPPUI::style::color(0xFFFFFF));
                    }
                    for(int i = 1; i <= 20; ++i)
                        for(auto box:CPPUI::VBox()
                            .style(CPPUI::style::bgColor(0xE88A2A))
                            .layout(CPPUI::layout::padding(8), CPPUI::layout::minHeight(30))) {
                            CPPUI::Label("Item " + std::to_string(i)).style(CPPUI::style::color(0x000000));
                        }
                }
            }
        }
    }
}

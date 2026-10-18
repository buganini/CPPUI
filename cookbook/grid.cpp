#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    namespace {
        void cell(const std::string & text, int row, int column, int rowSpan = 1) {
            auto & label = CPPUI::Label(text);
            label.id("grid-" + std::to_string(row) + "-" + std::to_string(column));
            label.grid_row = row; label.grid_column = column; label.grid_rowspan = rowSpan;
        }
    }
    void grid(Model &) {
        for(auto grid: CPPUI::Grid().id("example-grid")) {
            cell("R=0 C=0", 0, 0);
            cell("R=1 C=0", 1, 0);
            cell("R=0 C=1, rowspan=2", 0, 1, 2);
            cell("R=2 C=1", 2, 1);
        }
        CPPUI::Spacer();
    }
}

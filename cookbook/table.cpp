#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    namespace {
        void readonlyCell(const std::string & label) {
            auto & cell = CPPUI::TableNode(label).dblclick([label]() { std::cout << "on_dblclick " << label << '\n'; });
            cell.click([label]() { std::cout << "on_click " << label << '\n'; });
        }
    }
    void table(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Label("A1: " + static_cast<std::string>(model.tableA1));
            CPPUI::Label("B1: " + static_cast<std::string>(model.tableB1));
            CPPUI::Label("Sides: " + std::to_string(static_cast<int>(model.tableSides)));
            CPPUI::Label("Direction: " + static_cast<std::string>(model.tableDirection));
            CPPUI::Label("Price: $" + static_cast<std::string>(model.tablePrice) + " USD");
            for(auto table: CPPUI::Table().columnHeader({"Col 1", "Col 2", "Col 3", "Choice"})
                    .rowHeader({"Row 1", "Row 2", "Row 3"}).id("example-table")) {
                for(auto row: CPPUI::TableNode()) {
                    CPPUI::TableNode(model.tableA1).prefix("editable: ").display("A1: " + static_cast<std::string>(model.tableA1));
                    CPPUI::TableNode("A2").prefix("editable: ").set([&model](const std::string & value) { model.tableA1 = value; });
                    readonlyCell("A3");
                    CPPUI::TableNode(model.tableSides).prefix("editable: ").choices(std::vector<int>{3, 4, 5, 6, 8});
                }
                for(auto row: CPPUI::TableNode()) {
                    CPPUI::TableNode(model.tableB1).prefix("editable: ");
                    readonlyCell("B2"); readonlyCell("B3");
                    CPPUI::TableNode(model.tableDirection).prefix("editable: ").choices(std::vector<std::pair<std::string, std::string>>{
                        {"CW", "Clockwise"}, {"CCW", "Counterclockwise"}});
                }
                for(auto row: CPPUI::TableNode()) {
                    CPPUI::TableNode(model.tablePrice).prefix("editable: Price: $").suffix(" USD").placeholder("Enter price");
                    CPPUI::TableNode(""); CPPUI::TableNode(""); CPPUI::TableNode("");
                }
            }
        }
    }
}

#include "cppui/backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    namespace {
        class MatrixAdapter: public CPPUI::BaseTableAdapter<int> {
            std::shared_ptr<CPPUI::State<Model::Matrix>> values;
            public:
            explicit MatrixAdapter(std::shared_ptr<CPPUI::State<Model::Matrix>> values): values(std::move(values)) {}
            int data(int row, int column) const override { return static_cast<Model::Matrix>(*values).at(row).at(column); }
            void setData(int row, int column, const int & value) override {
                auto next = static_cast<Model::Matrix>(*values); next.at(row).at(column) = value; *values = next;
            }
            bool editable(int row, int) const override { return row < 3; }
            CPPUI::OptionalText rowHeader(int) const override { return {}; }
            int rowCount() const override { return static_cast<int>(static_cast<Model::Matrix>(*values).size()); }
            int columnCount() const override {
                const auto matrix = static_cast<Model::Matrix>(*values);
                return matrix.empty() ? 0 : static_cast<int>(matrix.front().size());
            }
            void clicked(int row, int column) override { std::cout << "clicked " << row << ' ' << column << '\n'; }
            void dblclicked(int row, int column) override { std::cout << "dblclicked " << row << ' ' << column << '\n'; }
        };
    }
    void tableAdapter(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Table(std::make_shared<MatrixAdapter>(model.adapterTable)).id("adapter-table-first");
            CPPUI::Table(std::make_shared<MatrixAdapter>(model.adapterTable)).id("adapter-table-second");
        }
    }
}

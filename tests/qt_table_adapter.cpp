#include "cppui/qt/table.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include "cppui/qt/layout.hpp"
#include <QAbstractItemModelTester>
#include <QPersistentModelIndex>
#include <sstream>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    using Matrix = std::vector<std::vector<int>>;
    struct Adapter: CPPUI::BaseTableAdapter<int> {
        std::shared_ptr<CPPUI::State<Matrix>> values{new CPPUI::State<Matrix>({{4, 9}, {3, 2}})};
        bool failRead = false, failWrite = false, failCount = false, headings = false;
        int clicks = 0, doubles = 0;
        std::function<void()> afterWrite, afterClick;
        int data(int row, int column) const override {
            if(failRead) throw std::runtime_error("read failure");
            return editData(row, column) * 10;
        }
        int editData(int row, int column) const override { return static_cast<Matrix>(*values).at(row).at(column); }
        void setData(int row, int column, const int & value) override {
            if(failWrite) throw std::runtime_error("write failure");
            auto next = static_cast<Matrix>(*values); next.at(row).at(column) = value; *values = next;
            if(afterWrite) afterWrite();
        }
        bool editable(int row, int) const override { return row == 0; }
        int rowCount() const override { return failCount ? -1 : static_cast<int>(static_cast<Matrix>(*values).size()); }
        int columnCount() const override { return 2; }
        CPPUI::OptionalText rowHeader(int row) const override { return headings ? CPPUI::OptionalText("R" + std::to_string(row)) : CPPUI::OptionalText(); }
        void clicked(int, int) override { ++clicks; if(afterClick) afterClick(); }
        void dblclicked(int, int) override { ++doubles; }
    };
    struct Root: CPPUI::Application {
        using Application::Application;
        std::shared_ptr<Adapter> source = std::make_shared<Adapter>();
        bool show = true, fallback = false, declarative = false;
        void content() override {
            for(auto window: CPPUI::Window()) for(auto column: CPPUI::VBox()) if(show) {
                if(declarative) {
                    for(auto table: CPPUI::Table().id("first")) for(auto row: CPPUI::TableNode()) CPPUI::TableNode(7);
                } else {
                    auto & first = CPPUI::Table(source); first.id("first");
                    if(fallback) first.rowHeader({"Fallback", "Fallback2"});
                    CPPUI::Table(source).id("second");
                }
            }
        }
        CPPUI::Qt::Table * table(int index = 0) { return static_cast<CPPUI::Qt::Table *>(children->front()->children->front()->children->at(index)); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync();
            auto native = static_cast<QTableView *>(root.table()->ui); auto model = native->model();
            QAbstractItemModelTester tester(model, QAbstractItemModelTester::FailureReportingMode::Fatal);
            require(model->rowCount() == 2 && model->columnCount() == 2, "adapter dimensions were lost");
            require(model->index(0, 0).data() == 40 && model->index(0, 0).data(::Qt::EditRole) == 4, "display/edit roles were conflated");
            require(model->headerData(1, ::Qt::Horizontal) == "Col 1" && native->verticalHeader()->isHidden(), "adapter headers were not applied");
            require(!model->setData(model->index(1, 0), 8), "readonly adapter cell accepted an edit");
            require(model->setData(model->index(0, 0), "8"), "typed edit failed"); settle();
            require(root.table()->ui == native && root.table()->model == model && root.table(1)->model->index(0, 0).data() == 80,
                "shared State did not refresh both adapter views or native identity changed");
            emit native->clicked(model->index(0, 0)); emit native->doubleClicked(model->index(0, 0));
            require(root.source->clicks == 1 && root.source->doubles == 1, "adapter callbacks were lost or duplicated");
            root.fallback = true; root.sync();
            require(!native->verticalHeader()->isHidden() && model->headerData(0, ::Qt::Vertical) == "Fallback", "declaration header fallback failed");
            root.source->headings = true; root.sync();
            require(model->headerData(0, ::Qt::Vertical) == "R0", "adapter header did not override fallback");
            QPersistentModelIndex index(model->index(0, 0));
            *root.source->values = Matrix{{1, 2}, {3, 4}, {5, 6}}; settle();
            require(model->rowCount() == 3 && !index.isValid(), "adapter growth did not reset indexes");
            std::ostringstream errors; auto old = std::cerr.rdbuf(errors.rdbuf());
            root.source->failRead = true; require(!model->index(0, 0).data().isValid(), "failing read produced data"); root.source->failRead = false;
            root.source->failWrite = true; const bool wrote = model->setData(model->index(0, 0), 9); root.source->failWrite = false;
            std::cerr.rdbuf(old);
            require(!wrote && errors.str().find("read failure") != std::string::npos && errors.str().find("write failure") != std::string::npos,
                "adapter errors escaped native callbacks");
            root.source->failCount = true; bool rejected = false;
            try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
            root.source->failCount = false;
            require(rejected && root.table()->ui == native && model->rowCount() == 3, "invalid adapter snapshot replaced mounted data");
            std::weak_ptr<Adapter> oldAdapter = root.source;
            root.source = std::make_shared<Adapter>(); root.sync();
            require(oldAdapter.expired() && model->index(0, 0).data() == 40, "adapter replacement retained the old source");
            root.declarative = true; root.sync();
            require(root.table()->ui == native && model->index(0, 0).data() == 7, "adapter-to-declaration switching failed");
            root.declarative = false; root.sync();
            root.source->afterWrite = [&]() { root.source.reset(); root.show = false; root.sync(); };
            oldAdapter = root.source;
            require(model->setData(model->index(0, 0), 12), "self-removing adapter edit did not finish");
            require(oldAdapter.expired() && model->rowCount() == 0, "retired model retained its adapter"); settle();
            for(int i = 0; i < 12; ++i) {
                root.source = std::make_shared<Adapter>(); root.show = true; root.sync();
                root.show = false; root.sync(); settle();
            }
            root.source = std::make_shared<Adapter>(); root.show = true; root.sync();
            auto declaration = root.table(); delete static_cast<QWidget *>(declaration->ui);
            require(!declaration->ui && !declaration->model, "native deletion left adapter observers dangling");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "adapter table teardown leaked roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

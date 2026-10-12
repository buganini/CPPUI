#include "cppui/qt/table.hpp"
#include "cppui/qt/application.hpp"
#include "cppui/qt/window.hpp"
#include <QLineEdit>
#include <QPersistentModelIndex>
#include <QSpinBox>
#include <sstream>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() { QCoreApplication::processEvents(); QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); }
    class Root: public CPPUI::Application {
        public:
        CPPUI::State<std::string> text{std::string("A")}, other{std::string("B")};
        CPPUI::State<int> number{4};
        CPPUI::State<bool> boolean{true};
        bool show = true, headers = true, empty = false, rebind = false, fail = false, selfRemove = false, immediate = false;
        int limit = -1, clicks = 0, doubles = 0, writes = 0;
        std::shared_ptr<int> capture = std::make_shared<int>(1);
        using Application::Application;
        void content() override {
            for(auto window: CPPUI::Window().size(500, 400)) if(show) {
                auto & table = CPPUI::Table();
                if(headers) table.columnHeader({"Name", "Choice", "Setter", "Number", "Bool"}).rowHeader({"R1", "R2", "R3"});
                if(limit >= 0) table.rowCount(limit).columnCount(limit);
                for(auto scope: table) if(!empty) {
                    for(auto row: CPPUI::TableNode()) {
                        CPPUI::TableNode(rebind ? other : text).display(std::string("value:") + static_cast<std::string>(rebind ? other : text));
                        CPPUI::TableNode(CPPUI::Binding<int>([this]() { return static_cast<int>(number); },
                            [this](const int & value) { number = value; if(immediate) sync(); }))
                            .choices(std::vector<std::pair<int, std::string>>{{3, "same"}, {4, "same"}, {5, "five"}});
                        CPPUI::TableNode("raw").set([this](const std::string & value) {
                            ++writes;
                            if(fail) throw std::runtime_error("rejected edit");
                            text = value;
                            if(selfRemove) { show = false; sync(); }
                        });
                        CPPUI::TableNode(number);
                        CPPUI::TableNode(boolean);
                    }
                    for(auto row: CPPUI::TableNode()) {
                        auto data = capture;
                        auto & cell = CPPUI::TableNode("fixed").dblclick([this, data]() { doubles += *data; });
                        cell.click([this, data]() { clicks += *data; if(selfRemove) { show = false; sync(); } });
                    }
                }
            }
        }
        CPPUI::Qt::Table * declaration() { return static_cast<CPPUI::Qt::Table *>(children->front()->children->front()); }
        QTableView * native() { return static_cast<QTableView *>(declaration()->ui); }
    };
}
int main(int argc, char ** argv) {
    QApplication qt(argc, argv);
    try {
        {
            Root root(argc, argv); root.sync(); settle();
            auto table = root.native(); auto model = table->model();
            require(model->rowCount() == 3 && model->columnCount() == 5, "table did not infer dimensions from rows and headers");
            require(!table->horizontalHeader()->isHidden() && model->headerData(4, ::Qt::Horizontal) == "Bool", "table headers were lost");
            require(!model->data(model->index(2, 4)).isValid() && model->flags(model->index(2, 4)) == ::Qt::NoItemFlags, "ragged cells were not inert");
            require(!model->setData(model->index(2, 4), "ignored") && !(model->flags(model->index(1, 0)) & ::Qt::ItemIsEditable), "missing or readonly cell accepted editing");
            require(model->rowCount(model->index(0, 0)) == 0 && model->columnCount(model->index(0, 0)) == 0, "table model exposed nested rows");
            require(model->data(model->index(0, 0)) == "value:A" && model->data(model->index(0, 0), ::Qt::EditRole) == "A", "display and edit roles were conflated");
            require(model->setData(model->index(0, 0), "edited") && static_cast<std::string>(root.text) == "edited", "string binding did not commit");
            settle(); require(root.native() == table && table->model() == model, "redraw replaced native table/model");
            require(model->data(model->index(0, 0)) == "value:edited", "bound display did not refresh");
            auto choiceIndex = model->index(0, 1);
            auto delegate = table->itemDelegate();
            QWidget holder;
            auto editor = dynamic_cast<CPPUI::Qt::TableChoiceEditor *>(delegate->createEditor(&holder, {}, choiceIndex));
            require(editor && editor->count() == 3, "choice editor did not build options");
            delegate->setEditorData(editor, choiceIndex);
            require(editor->currentIndex() == 1 && editor->itemData(0).metaType() == QMetaType::fromType<int>(), "choice values lost their integer type or duplicate labels matched incorrectly");
            int commits = 0, closes = 0;
            auto commit = QObject::connect(delegate, &QAbstractItemDelegate::commitData, &holder, [&](QWidget * value) {
                ++commits; delegate->setModelData(value, model, choiceIndex);
            });
            auto close = QObject::connect(delegate, &QAbstractItemDelegate::closeEditor, &holder, [&](QWidget *, QAbstractItemDelegate::EndEditHint) { ++closes; });
            editor->setCurrentIndex(0); emit editor->activated(0);
            require(root.number == 3 && commits == 1 && closes == 1, "choice activation did not commit and close once");
            QObject::disconnect(commit); QObject::disconnect(close); delete editor;
            settle();
            root.number = 17; settle(); choiceIndex = model->index(0, 1);
            editor = dynamic_cast<CPPUI::Qt::TableChoiceEditor *>(delegate->createEditor(&holder, {}, choiceIndex));
            delegate->setEditorData(editor, choiceIndex);
            require(editor->currentIndex() == -1 && editor->placeholderText() == "17", "unknown choice was not preserved as a placeholder");
            delegate->setModelData(editor, model, choiceIndex);
            require(root.number == 17, "unknown choice silently overwrote model value"); delete editor;
            root.immediate = true;
            table->edit(model->index(0, 1));
            auto nativeEditor = table->findChild<QComboBox *>();
            require(nativeEditor, "native table edit did not open a choice editor");
            nativeEditor->setCurrentIndex(2); emit nativeEditor->activated(2);
            require(root.number == 5, "native choice editor failed during a synchronous redraw");
            settle(); root.immediate = false;
            require(model->setData(model->index(0, 3), 8) && root.number == 8, "numeric cell binding did not commit");
            require(model->setData(model->index(0, 4), false) && !root.boolean, "boolean cell binding did not commit");
            settle();
            root.rebind = true; root.sync();
            require(model->setData(model->index(0, 0), "other") && static_cast<std::string>(root.other) == "other" && static_cast<std::string>(root.text) == "edited", "cell retained its old binding");
            settle();
            QPersistentModelIndex oldIndex(model->index(0, 0));
            std::weak_ptr<int> oldCapture = root.capture;
            root.capture = std::make_shared<int>(7); root.sync();
            require(!oldIndex.isValid() && oldCapture.expired(), "table reset retained an old index or callback capture");
            emit table->clicked(model->index(1, 0)); emit table->doubleClicked(model->index(1, 0));
            require(root.clicks == 7 && root.doubles == 7, "cell dispatch used an old or duplicate callback");
            root.fail = true;
            std::ostringstream errors; auto previous = std::cerr.rdbuf(errors.rdbuf());
            const bool accepted = model->setData(model->index(0, 2), "bad"); std::cerr.rdbuf(previous);
            require(!accepted && errors.str().find("rejected edit") != std::string::npos, "setter exception escaped native dispatch");
            root.fail = false;
            root.limit = 0; root.sync(); require(model->rowCount() == 0 && model->columnCount() == 0, "explicit zero dimensions were ignored");
            root.limit = 1; root.sync(); require(model->rowCount() == 1 && model->columnCount() == 1, "explicit dimensions were not applied");
            root.limit = -1; root.headers = false; root.empty = true; root.sync();
            require(model->rowCount() == 0 && model->columnCount() == 0 && table->horizontalHeader()->isHidden(), "empty table or omitted headers retained previous values");
            root.empty = false; root.sync(); require(model->rowCount() == 2 && model->columnCount() == 5, "ragged row width inference failed");
            root.selfRemove = true;
            require(model->setData(model->index(0, 2), "removed") && root.writes == 2, "self-removing setter did not finish");
            require(model->rowCount() == 0 && !model->setData(model->index(0, 0), "late"), "retired table model still dispatched edits"); settle();
            for(int i = 0; i < 10; ++i) { root.show = true; root.sync(); root.show = false; root.sync(); settle(); }
            root.show = true; root.sync(); table = root.native(); model = table->model();
            emit table->clicked(model->index(1, 0));
            require(root.clicks == 14 && model->rowCount() == 0, "self-removing click did not finish or deactivate the table"); settle();
            root.show = true; root.selfRemove = false; root.sync();
            auto declaration = root.declaration(); delete root.native();
            require(!declaration->ui && !declaration->model, "external deletion left table native observers dangling");
        }
        settle(); require(qt.topLevelWidgets().isEmpty(), "table teardown retained native roots");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

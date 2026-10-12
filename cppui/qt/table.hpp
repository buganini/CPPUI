#ifndef CPPUI_QT_TABLE
#define CPPUI_QT_TABLE

#include "base.hpp"
#include "../binding.hpp"
#include "../optional.hpp"
#include <QAbstractTableModel>
#include <QTableView>
#include <QHeaderView>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QPointer>
#include <QSignalBlocker>
#include <type_traits>
#include <limits>

namespace CPPUI { namespace Qt {
    namespace detail {
        template<class T> struct TableValue {
            static_assert(std::is_arithmetic<T>::value, "Table cells support arithmetic values and std::string");
            static QVariant encode(const T & value) { return QVariant::fromValue(value); }
            static T decode(QVariant value) {
                if(!value.convert(QMetaType::fromType<T>())) throw std::invalid_argument("Invalid table cell value");
                return value.value<T>();
            }
        };
        template<> struct TableValue<std::string> {
            static QVariant encode(const std::string & value) { return QString::fromStdString(value); }
            static std::string decode(const QVariant & value) { return value.toString().toStdString(); }
        };
        struct TableCell {
            QVariant value;
            OptionalText display;
            Optional<std::vector<std::pair<QVariant, QString>>> choices;
            std::function<QVariant()> read;
            std::function<void(const QVariant &)> write;
            std::function<void()> clicked, doubleClicked;
            QVariant current() const { return read ? read() : value; }
        };
        inline void tableError(const char * operation) {
            try { throw; }
            catch(const std::exception & error) { std::cerr << "CPPUI Table " << operation << ": " << error.what() << '\n'; }
            catch(...) { std::cerr << "CPPUI Table " << operation << ": unknown exception\n"; }
        }
    }

    class TableNodeBase: public Node {
        public:
        detail::TableCell cell;
        void destroy(bool) override {}
    };
    template<class T> class TableNode: public TableNodeBase {
        bool bound = false;
        public:
        explicit TableNode(const T & value) { cell.value = detail::TableValue<T>::encode(value); }
        explicit TableNode(Binding<T> model): TableNode(model.get()) {
            bound = true;
            cell.read = [model]() { return detail::TableValue<T>::encode(model.get()); };
            cell.write = [model](const QVariant & value) { model.set(detail::TableValue<T>::decode(value)); };
        }
        TableNode & display(const std::string & value) { cell.display = value; return *this; }
        TableNode & display(Binding<std::string> value) { return display(value.get()); }
        TableNode & set(std::function<void(const T &)> callback) {
            if(!bound) cell.write = callback ? std::function<void(const QVariant &)>([callback](const QVariant & value) {
                callback(detail::TableValue<T>::decode(value));
            }) : std::function<void(const QVariant &)>();
            return *this;
        }
        TableNode & choices(const std::vector<T> & values) {
            std::vector<std::pair<QVariant, QString>> items;
            for(const auto & value: values) {
                auto encoded = detail::TableValue<T>::encode(value);
                items.emplace_back(encoded, encoded.toString());
            }
            cell.choices = std::move(items); return *this;
        }
        TableNode & choices(const std::vector<std::pair<T, std::string>> & values) {
            std::vector<std::pair<QVariant, QString>> items;
            for(const auto & value: values) items.emplace_back(detail::TableValue<T>::encode(value.first), QString::fromStdString(value.second));
            cell.choices = std::move(items); return *this;
        }
        TableNode & dblclick(std::function<void()> callback) { cell.doubleClicked = std::move(callback); return *this; }
    };

    class TableModel: public QAbstractTableModel {
        std::vector<std::vector<detail::TableCell>> rows;
        Optional<std::vector<std::string>> columns, headings;
        int height = 0, width = 0;
        bool active = true;
        public:
        explicit TableModel(QObject * owner): QAbstractTableModel(owner) {}
        const detail::TableCell * cell(const QModelIndex & index) const {
            if(!active || index.model() != this || !index.isValid() || index.row() < 0 || index.column() < 0
                || index.row() >= height || index.column() >= width || index.row() >= static_cast<int>(rows.size())
                || index.column() >= static_cast<int>(rows[index.row()].size())) return nullptr;
            return &rows[index.row()][index.column()];
        }
        int rowCount(const QModelIndex & parent = {}) const override { return active && !parent.isValid() ? height : 0; }
        int columnCount(const QModelIndex & parent = {}) const override { return active && !parent.isValid() ? width : 0; }
        QVariant data(const QModelIndex & index, int role = ::Qt::DisplayRole) const override {
            const auto found = cell(index);
            if(!found || (role != ::Qt::DisplayRole && role != ::Qt::EditRole)) return {};
            // Reads and writes can invoke application code. Never keep a pointer
            // into storage that a synchronous redraw may replace.
            const auto entry = *found;
            try {
                if(role == ::Qt::DisplayRole && entry.display) return QString::fromStdString(*entry.display);
                const auto value = entry.current();
                if(role == ::Qt::DisplayRole && entry.choices)
                    for(const auto & choice: *entry.choices) if(choice.first == value) return choice.second;
                return value;
            } catch(...) { detail::tableError("read"); return {}; }
        }
        bool setData(const QModelIndex & index, const QVariant & value, int role = ::Qt::EditRole) override {
            const auto found = cell(index);
            if(role != ::Qt::EditRole || !found || !found->write) return false;
            auto write = found->write;
            try { write(value); return true; }
            catch(...) { detail::tableError("edit"); return false; }
        }
        ::Qt::ItemFlags flags(const QModelIndex & index) const override {
            const auto found = cell(index);
            if(!found) return ::Qt::NoItemFlags;
            return QAbstractTableModel::flags(index) | (found->write ? ::Qt::ItemIsEditable : ::Qt::NoItemFlags);
        }
        QVariant headerData(int section, ::Qt::Orientation orientation, int role) const override {
            if(!active || role != ::Qt::DisplayRole || section < 0) return {};
            const auto & labels = orientation == ::Qt::Horizontal ? columns : headings;
            return labels && section < static_cast<int>(labels->size()) ? QVariant(QString::fromStdString((*labels)[section])) : QVariant();
        }
        void reset(std::vector<std::vector<detail::TableCell>> values, Optional<std::vector<std::string>> columnLabels,
                   Optional<std::vector<std::string>> rowLabels, Optional<int> declaredRows, Optional<int> declaredColumns) {
            beginResetModel();
            rows = std::move(values); columns = std::move(columnLabels); headings = std::move(rowLabels);
            height = declaredRows ? *declaredRows : std::max(static_cast<int>(rows.size()), headings ? static_cast<int>(headings->size()) : 0);
            width = columns ? static_cast<int>(columns->size()) : 0;
            for(const auto & row: rows) width = std::max(width, static_cast<int>(row.size()));
            if(declaredColumns) width = *declaredColumns;
            active = true;
            endResetModel();
        }
        void deactivate() {
            active = false;
            beginResetModel(); rows.clear(); height = width = 0; endResetModel();
        }
        void dispatch(const QModelIndex & index, bool twice) {
            const auto found = cell(index);
            if(!found) return;
            auto invoke = twice ? found->doubleClicked : found->clicked;
            try { if(invoke) invoke(); } catch(...) { detail::tableError("callback"); }
        }
    };
    class TableChoiceEditor: public QComboBox {
        public:
        explicit TableChoiceEditor(QWidget * owner): QComboBox(owner) {}
    };
    class TableDelegate: public QStyledItemDelegate {
        public:
        explicit TableDelegate(QObject * owner): QStyledItemDelegate(owner) {}
        QWidget * createEditor(QWidget * parent, const QStyleOptionViewItem & option, const QModelIndex & index) const override {
            auto model = dynamic_cast<const TableModel *>(index.model());
            const auto cell = model ? model->cell(index) : nullptr;
            if(!cell || !cell->choices) return QStyledItemDelegate::createEditor(parent, option, index);
            auto editor = new TableChoiceEditor(parent);
            editor->setAutoFillBackground(true);
            for(const auto & choice: *cell->choices) editor->addItem(choice.second, choice.first);
            QPointer<TableDelegate> delegate(const_cast<TableDelegate *>(this));
            QPointer<QComboBox> alive(editor);
            QObject::connect(editor, &QComboBox::activated, editor, [delegate, alive](int) {
                if(!delegate || !alive) return;
                emit delegate->commitData(alive);
                if(delegate && alive) emit delegate->closeEditor(alive);
            });
            return editor;
        }
        void setEditorData(QWidget * editor, const QModelIndex & index) const override {
            if(auto combo = dynamic_cast<TableChoiceEditor *>(editor)) {
                QSignalBlocker blocked(combo);
                const auto value = index.model()->data(index, ::Qt::EditRole);
                combo->setCurrentIndex(combo->findData(value));
                if(combo->currentIndex() < 0) combo->setPlaceholderText(value.toString());
            } else QStyledItemDelegate::setEditorData(editor, index);
        }
        void setModelData(QWidget * editor, QAbstractItemModel * model, const QModelIndex & index) const override {
            if(auto combo = dynamic_cast<TableChoiceEditor *>(editor)) {
                if(combo->currentIndex() >= 0) model->setData(index, combo->currentData(), ::Qt::EditRole);
            } else QStyledItemDelegate::setModelData(editor, model, index);
        }
    };
    class Table: public BaseWidget {
        bool autofit;
        Optional<std::vector<std::string>> columns, headings;
        Optional<int> height, width;
        protected:
        void clearNative() override { model = nullptr; BaseWidget::clearNative(); }
        public:
        TableModel * model = nullptr;
        explicit Table(bool autofit = true): autofit(autofit) { is_terminal = true; }
        Table & columnHeader(const std::vector<std::string> & labels) { columns = labels; return *this; }
        Table & rowHeader(const std::vector<std::string> & labels) { headings = labels; return *this; }
        Table & rowCount(int value) { if(value < 0) throw std::invalid_argument("Table row count must be non-negative"); height = value; return *this; }
        Table & columnCount(int value) { if(value < 0) throw std::invalid_argument("Table column count must be non-negative"); width = value; return *this; }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Table *>(previous);
            if(prev && prev->ui) { ui = prev->ui; model = prev->model; }
            else {
                auto table = new QTableView(); ui = table;
                model = new TableModel(table); table->setModel(model);
                table->setItemDelegate(new TableDelegate(table));
            }
            auto table = static_cast<QTableView *>(ui);
            std::vector<std::vector<detail::TableCell>> rows;
            for(auto row: *children) {
                rows.emplace_back();
                for(auto node: *row->children) {
                    if(auto entry = dynamic_cast<TableNodeBase *>(node)) {
                        rows.back().push_back(entry->cell);
                        rows.back().back().clicked = entry->onClicked;
                    } else throw std::logic_error("Table rows require TableNode cells");
                }
            }
            model->reset(std::move(rows), columns, headings, height, width);
            table->horizontalHeader()->setVisible(bool(columns)); table->verticalHeader()->setVisible(bool(headings));
            if(autofit) table->resizeColumnsToContents();
            auto ownedModel = model;
            connectSignal(QObject::connect(table, &QTableView::clicked, model, [ownedModel](const QModelIndex & index) { ownedModel->dispatch(index, false); }));
            connectSignal(QObject::connect(table, &QTableView::doubleClicked, model, [ownedModel](const QModelIndex & index) { ownedModel->dispatch(index, true); }));
        }
        void destroy(bool direct) override {
            disconnectSignals();
            if(model) model->deactivate();
            BaseWidget::destroy(direct);
        }
    };
}}
namespace CPPUI {
    inline Qt::Table & Table(bool autofit = true) { return *new Qt::Table(autofit); }
    inline Qt::TableNode<std::string> & TableNode(const std::string & value = {}) { return *new Qt::TableNode<std::string>(value); }
    template<class T> inline Qt::TableNode<T> & TableNode(Binding<T> model) { return *new Qt::TableNode<T>(model); }
    template<class T> inline Qt::TableNode<T> & TableNode(State<T> & model) { return TableNode(Binding<T>(model)); }
    template<class T> inline typename std::enable_if<std::is_arithmetic<T>::value, Qt::TableNode<T> &>::type TableNode(T value) { return *new Qt::TableNode<T>(value); }
}
#endif

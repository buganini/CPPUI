#ifndef CPPUI_QT_COMBOBOX
#define CPPUI_QT_COMBOBOX

#include "base.hpp"
#include "../binding.hpp"
#include "../optional.hpp"
#include <QComboBox>
#include <QSignalBlocker>
#include <QPointer>
#include <QStringList>
#include <QCompleter>
#include <QAbstractItemView>
#include <QAbstractProxyModel>
#include <QLineEdit>

namespace CPPUI { namespace Qt {
    class ComboBoxItem;
    class ComboBoxWidget: public Widget<QComboBox> {
        public:
        enum { IdentityRole = ::Qt::UserRole + 1 };
        bool itemsChanged = false;
        Optional<int> declaredIndex;
        OptionalText declaredText;
        QVariant selectedKey;
        QString freeText;
        bool editing = false, committing = false, autoexpand = true;
        QPointer<QCompleter> candidates;
        void complete() {
            auto completion = candidates.data();
            if(!autoexpand || !isEnabled() || !isVisible() || !completion) return;
            if(count()) completion->complete();
            else completion->popup()->hide();
        }
        void showPopup() override {
            if(candidates) candidates->popup()->hide();
            QComboBox::showPopup();
        }
        void hideEvent(QHideEvent * event) override {
            if(candidates) candidates->popup()->hide();
            QComboBox::hideEvent(event);
        }
    };
    class ComboBox: public BaseWidget {
        bool editable;
        bool isDisabled = false;
        bool expand = true;
        Optional<Binding<int>> indexModel;
        Optional<Binding<std::string>> textModel;
        Optional<Binding<std::string>> editBinding;
        int indexValue = -1;
        std::string textValue;
        std::function<void()> onChange;
        std::function<void()> onInput;
        QString editorText;
        int editorCursor = -1, editorSelection = -1, editorSelectionLength = 0;
        public:
        ComboBox(bool editable = false, Optional<Binding<int>> indexModel = {}, Optional<Binding<std::string>> textModel = {})
            : editable(editable), indexModel(indexModel), textModel(textModel) {
            if(indexModel) indexValue = indexModel->get();
            if(textModel) textValue = textModel->get();
        }
        ComboBox & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
        ComboBox & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
        ComboBox & editModel(Binding<std::string> model) { editBinding = std::move(model); return *this; }
        ComboBox & autoexpand(bool value = true) { expand = value; return *this; }
        ComboBox & disabled(bool value = true) { isDisabled = value; return *this; }
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new ComboBoxWidget();
            QSignalBlocker block(ui);
            auto combo = static_cast<ComboBoxWidget *>(ui);
            if(auto editor = combo->lineEdit()) {
                editorText = editor->text(); editorCursor = editor->cursorPosition();
                editorSelection = editor->selectionStart(); editorSelectionLength = editor->selectedText().size();
            }
            combo->setEditable(editable);
            combo->setInsertPolicy(QComboBox::NoInsert);
            combo->autoexpand = expand;
            // Keep completion independent of QLineEdit's built-in completer:
            // that implementation inserts highlighted rows before acceptance.
            if(editable && combo->completer()) combo->setCompleter(nullptr);
            if(editable && expand) {
                if(!combo->candidates) {
                    auto completion = new QCompleter(combo->model(), combo);
                    combo->candidates = completion;
                    completion->setCompletionColumn(combo->modelColumn());
                    completion->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
                    QPointer<ComboBoxWidget> alive(combo);
                    QObject::connect(completion, QOverload<const QModelIndex &>::of(&QCompleter::activated), combo,
                        [alive](const QModelIndex & index) {
                            if(!alive || !alive->isEnabled() || !alive->candidates || !index.isValid()) return;
                            auto proxy = qobject_cast<QAbstractProxyModel *>(alive->candidates->completionModel());
                            if(!proxy) return;
                            auto source = proxy->mapToSource(index);
                            if(source.isValid() && source.model() == alive->model()) alive->setCurrentIndex(source.row());
                        });
                }
                if(combo->candidates->widget() != combo->lineEdit()) combo->candidates->setWidget(combo->lineEdit());
            } else if(combo->candidates) delete combo->candidates.data();
            if(isDisabled && combo->candidates) combo->candidates->popup()->hide();
            ui->setDisabled(isDisabled);
        }
        void addChild(int index, Node * child) override;
        void removeChild(int index, Node *) override {
            if(ui) {
                QSignalBlocker block(ui);
                auto combo = static_cast<ComboBoxWidget *>(ui);
                combo->removeItem(index); combo->itemsChanged = true;
            }
        }
        void postSync() override {
            if(!ui) return;
            auto combo = static_cast<ComboBoxWidget *>(ui);
            disconnectSignals();
            if(!combo->committing) {
                QSignalBlocker block(combo);
                int index = -1;
                QString text;
                if(indexModel) {
                    if(indexValue >= 0 && indexValue < combo->count()) { index = indexValue; text = combo->itemText(index); }
                    else if(editable) text = textModel ? QString::fromStdString(textValue) : combo->freeText;
                } else if(textModel) {
                    text = QString::fromStdString(textValue);
                    if(!editable || !combo->editing || text != combo->freeText) index = combo->findData(text);
                    if(index >= 0) text = combo->itemText(index);
                } else {
                    if(combo->selectedKey.isValid()) index = combo->findData(combo->selectedKey, ComboBoxWidget::IdentityRole);
                    if(index >= 0) text = combo->itemText(index);
                    else if(editable) text = combo->freeText;
                }
                auto editor = combo->lineEdit();
                const auto oldText = editor ? editor->text() : QString();
                const int cursor = editor ? editor->cursorPosition() : -1;
                const int selection = editor ? editor->selectionStart() : -1;
                const int length = editor ? editor->selectedText().size() : 0;
                if(combo->currentIndex() != index) combo->setCurrentIndex(index);
                // Read-only text follows the selected index. setCurrentText()
                // would select the first duplicate label and lose value identity.
                if(editable && combo->currentText() != text) combo->setCurrentText(text);
                if(editor && oldText == text) {
                    if(selection >= 0 && length) editor->setSelection(selection, length);
                    else editor->setCursorPosition(cursor);
                } else if(editor && editorCursor >= 0 && editorText == text) {
                    if(editorSelection >= 0 && editorSelectionLength) editor->setSelection(editorSelection, editorSelectionLength);
                    else editor->setCursorPosition(editorCursor);
                }
                combo->declaredIndex = index;
                combo->declaredText = text.toStdString();
            }
            combo->itemsChanged = false;
            if(combo->candidates && !combo->count()) combo->candidates->popup()->hide();
            auto indexBinding = indexModel;
            auto textBinding = textModel;
            auto callback = onChange;
            QPointer<ComboBoxWidget> alive(combo);
            connectSignal(QObject::connect(combo, &QComboBox::currentIndexChanged, combo, [alive, indexBinding, textBinding, callback](int index) {
                auto indices = indexBinding;
                auto texts = textBinding;
                auto invoke = callback;
                if(!alive) return;
                const bool valid = index >= 0 && index < alive->count();
                const auto value = valid ? alive->itemData(index).toString().toStdString() : std::string();
                // Save native state before a binding can redraw or remove us.
                alive->selectedKey = valid ? alive->itemData(index, ComboBoxWidget::IdentityRole) : QVariant();
                alive->freeText = valid ? alive->itemText(index) : QString();
                alive->editing = false;
                try {
                    if(indices) indices->set(index);
                    if(texts) texts->set(value);
                    if(invoke) invoke();
                } catch(const std::exception & error) { std::cerr << "CPPUI ComboBox index: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI ComboBox index: unknown exception\n"; }
            }));
            auto draft = editBinding; auto inputCallback = onInput;
            auto edited = [alive, indexBinding, textBinding, callback, draft, inputCallback](const QString & text, bool user) {
                auto indices = indexBinding;
                auto texts = textBinding;
                auto invoke = callback;
                auto edits = draft; auto input = inputCallback;
                if(!alive || alive->committing || !alive->isEnabled()) return;
                const int index = alive->currentIndex();
                // A native selection also changes its displayed label; commit
                // the item's value only through currentIndexChanged.
                if(!user && index >= 0 && index < alive->count() && text == alive->itemText(index)) return;
                const auto value = text.toStdString();
                const bool changed = !alive->editing || alive->freeText != text;
                alive->selectedKey.clear();
                alive->freeText = text;
                alive->editing = true; alive->committing = true;
                {
                    QSignalBlocker block(alive.data());
                    auto editor = alive->lineEdit();
                    const int cursor = editor->cursorPosition();
                    alive->setCurrentIndex(-1); alive->setEditText(text); editor->setCursorPosition(cursor);
                }
                try {
                    if(changed) { if(indices) indices->set(-1); if(texts) texts->set(value); }
                    if(user) { if(edits) edits->set(value); if(input) input(); }
                    if(changed && invoke) invoke();
                }
                catch(const std::exception & error) { std::cerr << "CPPUI ComboBox text: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI ComboBox text: unknown exception\n"; }
                if(alive) {
                    alive->committing = false;
                    // The native model already contains the latest declared rows.
                    if(user) alive->complete();
                }
            };
            if(editable) {
                connectSignal(QObject::connect(combo, &QComboBox::currentTextChanged, combo, [edited](const QString & text) { edited(text, false); }));
                connectSignal(QObject::connect(combo->lineEdit(), &QLineEdit::textEdited, combo, [edited](const QString & text) { edited(text, true); }));
            }
        }
    };
    class ComboBoxItem: public Node {
        public:
        std::string text, value;
        explicit ComboBoxItem(const std::string & text): ComboBoxItem(text, text) {}
        ComboBoxItem(const std::string & text, const std::string & value): text(text), value(value) { id(text); is_terminal = true; }
        QStringList identity() const {
            // Include virtual scopes so equal item IDs in different subviews
            // remain distinct, without retaining declaration pointers.
            QStringList key;
            for(auto node = static_cast<const Node *>(this); node && node != dom_parent; node = node->parent) {
                QString part = QString::number(static_cast<qulonglong>(node->typecode()));
                if(node->grid_row >= 0 && node->grid_column >= 0)
                    part += QString(":%1,%2,%3,%4").arg(node->grid_row).arg(node->grid_column).arg(node->grid_rowspan).arg(node->grid_columnspan);
                part += node->has_id ? ":id:" + QString::fromStdString(node->_id) : ":none";
                key.prepend(part);
                if(node == node->parent) break;
            }
            return key;
        }
        void destroy(bool) override {} // The native ComboBox owns its item data.
        void postUpdate() override {
            auto parent = dynamic_cast<ComboBox *>(dom_parent);
            if(!parent || !parent->ui) return;
            int index = 0;
            parent->findDomOffsetForNode(this, &index);
            auto combo = static_cast<ComboBoxWidget *>(parent->ui);
            {
                QSignalBlocker block(combo);
                if(combo->itemText(index) != QString::fromStdString(text)
                    || combo->itemData(index).toString() != QString::fromStdString(value)) combo->itemsChanged = true;
                combo->setItemText(index, QString::fromStdString(text));
                combo->setItemData(index, QString::fromStdString(value));
                combo->setItemData(index, identity(), ComboBoxWidget::IdentityRole);
            }
            // Independent virtual views can update items without rebuilding the
            // ComboBox declaration. Refresh selection after their items settle.
            parent->postSync();
        }
    };
    inline void ComboBox::addChild(int index, Node * child) {
        if(auto item = dynamic_cast<ComboBoxItem *>(child)) {
            QSignalBlocker block(ui);
            auto combo = static_cast<ComboBoxWidget *>(ui);
            combo->insertItem(index, QString::fromStdString(item->text), QString::fromStdString(item->value));
            combo->setItemData(index, item->identity(), ComboBoxWidget::IdentityRole);
            combo->itemsChanged = true;
        }
    }
}}
namespace CPPUI {
    inline Qt::ComboBox & ComboBox(bool editable = false) { return *new Qt::ComboBox(editable); }
    inline Qt::ComboBox & ComboBox(bool editable, Binding<int> index) { return *new Qt::ComboBox(editable, index); }
    inline Qt::ComboBox & ComboBox(bool editable, Binding<std::string> text) { return *new Qt::ComboBox(editable, {}, text); }
    inline Qt::ComboBox & ComboBox(bool editable, Binding<int> index, Binding<std::string> text) { return *new Qt::ComboBox(editable, index, text); }
    inline Qt::ComboBoxItem & ComboBoxItem(const std::string & text) { return *new Qt::ComboBoxItem(text); }
    inline Qt::ComboBoxItem & ComboBoxItem(const std::string & text, const std::string & value) { return *new Qt::ComboBoxItem(text, value); }
}
#endif

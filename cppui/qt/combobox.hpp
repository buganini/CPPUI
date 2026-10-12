#ifndef CPPUI_QT_COMBOBOX
#define CPPUI_QT_COMBOBOX

#include "base.hpp"
#include "../binding.hpp"
#include "../optional.hpp"
#include <QComboBox>
#include <QSignalBlocker>
#include <QPointer>

namespace CPPUI { namespace Qt {
    class ComboBoxItem;
    class ComboBoxWidget: public QComboBox {
        public:
        bool itemsChanged = false;
        Optional<int> declaredIndex;
        OptionalText declaredText;
    };
    class ComboBox: public BaseWidget {
        bool editable;
        Optional<Binding<int>> indexModel;
        Optional<Binding<std::string>> textModel;
        int indexValue = -1;
        std::string textValue;
        std::function<void()> onChange;
        public:
        ComboBox(bool editable = false, Optional<Binding<int>> indexModel = {}, Optional<Binding<std::string>> textModel = {})
            : editable(editable), indexModel(indexModel), textModel(textModel) {
            if(indexModel) indexValue = indexModel->get();
            if(textModel) textValue = textModel->get();
        }
        ComboBox & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new ComboBoxWidget();
            QSignalBlocker block(ui);
            static_cast<QComboBox *>(ui)->setEditable(editable);
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
            if(indexModel || textModel || !combo->declaredIndex) {
                QSignalBlocker block(combo);
                int index = -1;
                QString text;
                if(indexModel) {
                    if(indexValue >= 0 && indexValue < combo->count()) { index = indexValue; text = combo->itemText(index); }
                } else if(textModel) {
                    text = QString::fromStdString(textValue);
                    index = combo->findData(text);
                }
                if(combo->itemsChanged || !combo->declaredIndex || *combo->declaredIndex != index) combo->setCurrentIndex(index);
                if(combo->itemsChanged || !combo->declaredText || *combo->declaredText != text.toStdString()) combo->setCurrentText(text);
                combo->declaredIndex = index;
                combo->declaredText = text.toStdString();
            }
            combo->itemsChanged = false;
            auto indexBinding = indexModel;
            auto textBinding = textModel;
            auto callback = onChange;
            QPointer<ComboBoxWidget> alive(combo);
            connectSignal(QObject::connect(combo, &QComboBox::currentIndexChanged, combo, [alive, indexBinding, textBinding, callback](int index) {
                auto indices = indexBinding;
                auto texts = textBinding;
                auto invoke = callback;
                const auto value = alive && index >= 0 && index < alive->count() ? alive->itemData(index).toString().toStdString() : std::string();
                try {
                    if(indices) indices->set(index);
                    if(texts) texts->set(value);
                    if(invoke) invoke();
                } catch(const std::exception & error) { std::cerr << "CPPUI ComboBox index: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI ComboBox index: unknown exception\n"; }
            }));
            if(editable) connectSignal(QObject::connect(combo, &QComboBox::currentTextChanged, combo, [textBinding, callback](const QString & text) {
                auto texts = textBinding;
                auto invoke = callback;
                const auto value = text.toStdString();
                try { if(texts) texts->set(value); if(invoke) invoke(); }
                catch(const std::exception & error) { std::cerr << "CPPUI ComboBox text: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI ComboBox text: unknown exception\n"; }
            }));
        }
    };
    class ComboBoxItem: public Node {
        public:
        std::string text, value;
        explicit ComboBoxItem(const std::string & text): ComboBoxItem(text, text) {}
        ComboBoxItem(const std::string & text, const std::string & value): text(text), value(value) { id(text); is_terminal = true; }
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

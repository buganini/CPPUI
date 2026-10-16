#ifndef CPPUI_QT_TEXTFIELD
#define CPPUI_QT_TEXTFIELD

#include "base.hpp"
#include "../binding.hpp"
#include "../optional.hpp"
#include <QLineEdit>
#include <QPointer>
#include <QSignalBlocker>

namespace CPPUI { namespace Qt {
    class TextFieldWidget: public Widget<QLineEdit> {
        public:
        bool editing = false, committing = false;
        OptionalText modelValue;
    };
    class TextField: public BaseWidget {
        Binding<std::string> model;
        Optional<Binding<std::string>> editModel;
        std::string value, editValue, placeholderText;
        protected:
        bool masked = false;
        private:
        std::function<void()> onInput, onChange;
        public:
        explicit TextField(Binding<std::string> model): model(model), value(model.get()) { is_terminal = true; }
        TextField(Binding<std::string> model, Binding<std::string> editModel)
            : TextField(model) { this->editModel = editModel; editValue = editModel.get(); }
        TextField & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
        TextField & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
        TextField & placeholder(const std::string & text) { placeholderText = text; return *this; }
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new TextFieldWidget();
            auto editor = static_cast<TextFieldWidget *>(ui);
            editor->setEchoMode(masked ? QLineEdit::Password : QLineEdit::Normal);
            editor->setPlaceholderText(QString::fromStdString(placeholderText));
            editor->setFocusPolicy(::Qt::ClickFocus);
            if(!editor->editing && (!editor->modelValue || *editor->modelValue != value)) {
                QSignalBlocker block(editor);
                editor->setText(QString::fromStdString(value));
                editor->modelValue = value;
            }
            if(editModel && !editor->editing && editValue != value) {
                try { editModel->set(value); }
                catch(const std::exception & error) { std::cerr << "CPPUI TextField edit model: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI TextField edit model: unknown exception\n"; }
            }
            auto draft = editModel;
            auto input = onInput;
            QPointer<TextFieldWidget> alive(editor);
            connectSignal(QObject::connect(editor, &QLineEdit::textChanged, editor, [alive, draft, input](const QString & text) {
                auto currentDraft = draft;
                auto invoke = input;
                const auto value = text.toStdString();
                if(!alive) return;
                alive->editing = true;
                try {
                    if(currentDraft) currentDraft->set(value);
                    if(invoke) invoke();
                } catch(const std::exception & error) { std::cerr << "CPPUI TextField input: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI TextField input: unknown exception\n"; }
            }));
            auto binding = model;
            auto change = onChange;
            connectSignal(QObject::connect(editor, &QLineEdit::editingFinished, editor, [alive, binding, draft, change]() {
                auto current = binding;
                auto currentDraft = draft;
                auto invoke = change;
                if(!alive || alive->committing) return;
                alive->committing = true;
                alive->editing = false;
                const auto value = alive->text().toStdString();
                try {
                    current.set(value);
                    if(currentDraft) currentDraft->set(value);
                    const auto committed = current.get();
                    if(alive) {
                        QSignalBlocker block(alive.data());
                        alive->setText(QString::fromStdString(committed));
                        alive->modelValue = committed;
                    }
                    if(invoke) invoke();
                    if(alive) alive->clearFocus();
                } catch(const std::exception & error) { std::cerr << "CPPUI TextField commit: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI TextField commit: unknown exception\n"; }
                if(alive) alive->committing = false;
            }));
        }
    };
    class MaskedField: public TextField {
        public:
        explicit MaskedField(Binding<std::string> model): TextField(model) { masked = true; }
        MaskedField(Binding<std::string> model, Binding<std::string> edit): TextField(model, edit) { masked = true; }
    };
}}
namespace CPPUI {
    inline Qt::TextField & TextField(Binding<std::string> model) { return *new Qt::TextField(model); }
    inline Qt::TextField & TextField(Binding<std::string> model, Binding<std::string> editModel) { return *new Qt::TextField(model, editModel); }
    inline Qt::MaskedField & MaskedField(Binding<std::string> model) { return *new Qt::MaskedField(model); }
    inline Qt::MaskedField & MaskedField(Binding<std::string> model, Binding<std::string> edit) { return *new Qt::MaskedField(model, edit); }
}
#endif

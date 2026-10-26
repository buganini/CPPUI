#ifndef CPPUI_QT_TEXTEDIT
#define CPPUI_QT_TEXTEDIT

#include "base.hpp"
#include "../binding.hpp"
#include "../optional.hpp"
#include <QPlainTextEdit>
#include <QPointer>
#include <QSignalBlocker>

namespace CPPUI { namespace Qt {
    class TextEditWidget: public Widget<QPlainTextEdit> {
        public:
        bool editing = false, committing = false;
        OptionalText modelValue;
        std::function<void()> finish;
        void focusOutEvent(QFocusEvent * event) override {
            Widget<QPlainTextEdit>::focusOutEvent(event);
            auto callback=finish;if(callback) callback();
        }
    };
    class TextEdit: public BaseWidget {
        Binding<std::string> model;
        Optional<Binding<std::string>> editModel;
        std::string value, editValue, placeholderText;
        protected:
        bool isReadonly=false,wrapText=true;
        private:
        std::function<void()> onInput, onChange;
        public:
        explicit TextEdit(Binding<std::string> model): model(model), value(model.get()) { is_terminal = true; expandX=expandY=4; }
        TextEdit(Binding<std::string> model, Binding<std::string> editModel)
            : TextEdit(model) { this->editModel = editModel; editValue = editModel.get(); }
        TextEdit & input(std::function<void()> callback) { onInput = std::move(callback); return *this; }
        TextEdit & change(std::function<void()> callback) { onChange = std::move(callback); return *this; }
        TextEdit & placeholder(const std::string & text) { placeholderText = text; return *this; }
        TextEdit & readonly(bool value=true) { isReadonly=value;return *this; }
        TextEdit & wrap(bool value=true) { wrapText=value;return *this; }
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new TextEditWidget();
            auto editor = static_cast<TextEditWidget *>(ui);
            editor->setReadOnly(isReadonly);editor->setTabChangesFocus(true);
            editor->setLineWrapMode(wrapText ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
            if(isReadonly) editor->editing=false;
            editor->setPlaceholderText(QString::fromStdString(placeholderText));
            editor->setFocusPolicy(::Qt::StrongFocus);
            if(!editor->editing && editor->toPlainText().toStdString()!=value) {
                QSignalBlocker block(editor);
                editor->setPlainText(QString::fromStdString(value));
                editor->modelValue = value;
            }
            if(editModel && !editor->editing && editValue != value) {
                try { editModel->set(value); }
                catch(const std::exception & error) { std::cerr << "CPPUI TextEdit edit model: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI TextEdit edit model: unknown exception\n"; }
            }
            auto draft = editModel;
            auto input = onInput;
            QPointer<TextEditWidget> alive(editor);
            const bool readonly=isReadonly;
            connectSignal(QObject::connect(editor, &QPlainTextEdit::textChanged, editor, [alive, draft, input, readonly]() {
                auto currentDraft = draft;
                auto invoke = input;
                if(!alive || readonly) return;
                const auto value=alive->toPlainText().toStdString();
                alive->editing = true;
                try {
                    if(currentDraft) currentDraft->set(value);
                    if(invoke) invoke();
                } catch(const std::exception & error) { std::cerr << "CPPUI TextEdit input: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI TextEdit input: unknown exception\n"; }
            }));
            auto binding = model;
            auto change = onChange;
            editor->finish=[alive, binding, draft, change, readonly]() {
                auto current = binding;
                auto currentDraft = draft;
                auto invoke = change;
                if(!alive || readonly || !alive->editing || alive->committing) return;
                alive->committing = true;
                alive->editing = false;
                const auto value = alive->toPlainText().toStdString();
                try {
                    current.set(value);
                    const auto committed = current.get();
                    if(currentDraft) currentDraft->set(committed);
                    if(alive) {
                        QSignalBlocker block(alive.data());
                        alive->setPlainText(QString::fromStdString(committed));
                        alive->modelValue = committed;
                    }
                    if(invoke) invoke();

                } catch(const std::exception & error) { std::cerr << "CPPUI TextEdit commit: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI TextEdit commit: unknown exception\n"; }
                if(alive) alive->committing = false;
            };
        }
    };
}}
namespace CPPUI {
    inline Qt::TextEdit & TextEdit(Binding<std::string> model) { return *new Qt::TextEdit(model); }
    inline Qt::TextEdit & TextEdit(Binding<std::string> model,Binding<std::string> edit) { return *new Qt::TextEdit(model,edit); }
}
#endif

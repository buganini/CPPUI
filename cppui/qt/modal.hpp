#ifndef CPPUI_QT_MODAL
#define CPPUI_QT_MODAL

#include "base.hpp"
#include "../binding.hpp"
#include "../optional.hpp"
#include <QDialog>
#include <QVBoxLayout>

namespace CPPUI { namespace Qt {
    class ModalWidget: public QDialog {
        bool openState = false;
        public:
        Optional<QSize> declaredSize;
        std::function<void(bool)> changed;
        std::function<void()> writeOff;

        explicit ModalWidget(QWidget * parent): QDialog(parent) { setModal(true); }

        void setOpen(bool value) {
            if(openState == value) return;
            openState = value;
            setVisible(value);
            auto notify = changed;
            if(notify) notify(value);
        }
        void done(int result) override {
            auto write = writeOff;
            setOpen(false);
            QDialog::done(result);
            // Binding writes can redraw or remove this dialog. Do not touch
            // the declaration or native widget after invoking the snapshot.
            try { if(write) write(); }
            catch(const std::exception & error) { std::cerr << "CPPUI Modal close: " << error.what() << '\n'; }
            catch(...) { std::cerr << "CPPUI Modal close: unknown exception\n"; }
        }
    };

    class Modal: public BaseWidget {
        bool visible;
        std::function<void()> writeOff;
        std::function<void()> onOpen, onClose;
        OptionalText caption;
        Optional<QSize> dimensions;
        Optional<bool> initialMaximized, initialFullscreen;
        QVBoxLayout * layout = nullptr;

        static void windowFlag(QWidget * widget, ::Qt::WindowState flag, bool value) {
            widget->setWindowState(value ? widget->windowState() | flag : widget->windowState() & ~flag);
        }
        protected:
        void clearNative() override {
            layout = nullptr;
            BaseWidget::clearNative();
        }

        public:
        template<class T>
        explicit Modal(Binding<T> status, T offValue = T{})
            : visible(static_cast<bool>(status.get())),
              writeOff([status, offValue]() { status.set(offValue); }) {
            if(!dynamic_cast<QtView *>(cppui_root)) {
                throw std::logic_error("Modal requires a Qt Application or QtView");
            }
            is_outoforder = true;
        }
        template<class T>
        explicit Modal(State<T> & status, T offValue = T{}): Modal(Binding<T>(status), offValue) {}

        Modal & title(const std::string & value) { caption = value; return *this; }
        Modal & size(int width, int height) { dimensions = QSize(width, height); return *this; }
        Modal & maximize(bool value = true) {
            if(ui) windowFlag(ui, ::Qt::WindowMaximized, value);
            else initialMaximized = value;
            return *this;
        }
        Modal & fullscreen(bool value = true) {
            if(ui) windowFlag(ui, ::Qt::WindowFullScreen, value);
            else initialFullscreen = value;
            return *this;
        }
        Modal & open(std::function<void()> callback) { onOpen = std::move(callback); return *this; }
        Modal & close(std::function<void()> callback) { onClose = std::move(callback); return *this; }

        void update(BaseWidget * previous) override {
            const bool creating = !previous || !previous->ui;
            if(creating) {
                auto owner = dynamic_cast<BaseWidget *>(dom_parent);
                ui = new ModalWidget(owner ? owner->ui : nullptr);
                layout = new QVBoxLayout(ui);
                layout->setContentsMargins(0, 0, 0, 0);
            } else {
                ui = previous->ui;
                layout = static_cast<Modal *>(previous)->layout;
            }
            auto dialog = static_cast<ModalWidget *>(ui);
            auto root = dynamic_cast<QtView *>(cppui_root);
            auto opened = onOpen;
            auto closed = onClose;
            dialog->changed = [root, opened, closed](bool opening) {
                if(root) root->queueCallback(opening ? opened : closed);
            };
            dialog->writeOff = writeOff;
            if(dimensions && (!dialog->declaredSize || *dimensions != *dialog->declaredSize)) {
                dialog->resize(*dimensions);
            }
            dialog->declaredSize = dimensions;
            if(caption) dialog->setWindowTitle(QString::fromStdString(*caption));
            if(creating) {
                if(initialMaximized) windowFlag(dialog, ::Qt::WindowMaximized, *initialMaximized);
                if(initialFullscreen) windowFlag(dialog, ::Qt::WindowFullScreen, *initialFullscreen);
            }
            dialog->setOpen(visible);
        }
        void * inner() override { return layout; }
        void addChild(int index, Node * child) override {
            if(!child->is_outoforder) layout->insertWidget(index, static_cast<QWidget *>(child->outer()));
        }
        void removeChild(int, Node * child) override {
            if(!child->is_outoforder) {
                if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
        }
        void destroy(bool direct) override {
            if(ui) {
                auto dialog = static_cast<ModalWidget *>(ui);
                dialog->setOpen(false);
                dialog->changed = {};
                dialog->writeOff = {};
            }
            BaseWidget::destroy(direct);
        }
    };
}}

namespace CPPUI {
    template<class T>
    inline Qt::Modal & Modal(Binding<T> status, T offValue = T{}) { return *new Qt::Modal(status, offValue); }
    template<class T>
    inline Qt::Modal & Modal(State<T> & status, T offValue = T{}) { return *new Qt::Modal(status, offValue); }
}

#endif

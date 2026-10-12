#ifndef CPPUI_QT_BASE
#define CPPUI_QT_BASE

#include "../view.hpp"
#include <QWidget>
#include <QBoxLayout>
#include <QCoreApplication>

namespace CPPUI {
    namespace Qt {
        class QtView: public View {
            QObject callbackContext;
            bool dirty = false, updating = false, scheduled = false, setupDone = false;

            public:
            ~QtView() override { clearChildren(); }

            virtual void setup() {}

            void update(Node * previous) override {
                if(destroyed || retired_by) return;
                // A nested view is first built by its parent's reconciliation.
                const bool ownUpdate = !updating;
                if(ownUpdate) updating = true;
                try {
                    if(!setupDone) { setup(); setupDone = true; }
                    View::update(previous);
                } catch(...) {
                    if(ownUpdate) { updating = false; dirty = false; }
                    throw;
                }
                if(ownUpdate) {
                    updating = false;
                    if(dirty) redraw();
                }
            }

            void redraw() override {
                if(destroyed || retired_by) return;
                dirty = true;
                if(updating || scheduled) return;
                scheduled = true;
                QMetaObject::invokeMethod(&callbackContext, [this]() {
                    scheduled = false;
                    if(!dirty) return;
                    try { sync(); }
                    catch(const std::exception & error) { std::cerr << "CPPUI redraw: " << error.what() << '\n'; }
                    catch(...) { std::cerr << "CPPUI redraw: unknown exception\n"; }
                }, ::Qt::QueuedConnection);
            }

            // Immediate synchronization for startup and explicit test flushing.
            // Exceptions propagate to the caller; queued redraws report them.
            void sync() {
                if(destroyed || retired_by) return;
                if(updating) { dirty = true; return; }
                dirty = false;
                updating = true;
                try { View::redraw(); }
                catch(...) { updating = false; dirty = false; throw; }
                updating = false;
                if(dirty) redraw();
            }

            void run() override { sync(); start(); }

            void clearChildren() {
                destroyed = true;
                dirty = false;
                scheduled = false;
                QCoreApplication::removePostedEvents(&callbackContext);
                for(Node * child: *children) recur_delete(child, true);
                children->clear();
            }

            // Virtual views own declarations, never a QWidget or QApplication.
            void destroy(bool) override {}

            // Lifecycle notifications run after reconciliation. Their context
            // belongs to the view, so removing that view cancels pending work.
            void queueCallback(std::function<void()> callback) {
                if(destroyed || !callback) return;
                QMetaObject::invokeMethod(&callbackContext, [callback]() {
                    try { callback(); }
                    catch(const std::exception & error) { std::cerr << "CPPUI callback: " << error.what() << '\n'; }
                    catch(...) { std::cerr << "CPPUI callback: unknown exception\n"; }
                }, ::Qt::QueuedConnection);
            }
        };

        class BaseWidget: public Node {
            QMetaObject::Connection nativeDestroyed;
            std::vector<QMetaObject::Connection> connections;

            protected:
            void connectSignal(const QMetaObject::Connection & connection) {
                connections.push_back(connection);
            }
            void disconnectSignals() {
                for(const auto & connection: connections) QObject::disconnect(connection);
                connections.clear();
            }
            virtual void clearNative() { ui = nullptr; }
            void releaseNativeAccess() {
                disconnectSignals();
                QObject::disconnect(nativeDestroyed);
                nativeDestroyed = {};
                clearNative();
            }

            public:
            // This observer is valid only within the explicit native lifetime.
            QWidget * ui = nullptr;

            ~BaseWidget() override { releaseNativeAccess(); }

            void update(Node * previous) override {
                auto prev = static_cast<BaseWidget *>(previous);
                if(prev) prev->disconnectSignals();
                update(prev);
                if(ui) {
                    nativeDestroyed = QObject::connect(ui, &QObject::destroyed, [this]() {
                        clearNative();
                    });
                }
                Node::update(previous);
            }

            virtual void update(BaseWidget * prev) = 0;

            void * inner() override { return ui; }
            void * outer() override { return ui; }

            void releaseRetiredRefs() override {
                releaseNativeAccess();
                Node::releaseRetiredRefs();
            }

            void destroy(bool direct) override {
                QWidget * widget = ui;
                releaseNativeAccess();
                if(direct && widget) widget->deleteLater();
            }
        };

        class Spacer: public Node {
            QMetaObject::Connection nativeDestroyed;
            QBoxLayout * owner = nullptr;
            void release() {
                QObject::disconnect(nativeDestroyed);
                nativeDestroyed = {};
                owner = nullptr;
                ui = nullptr;
            }
            public:
            QSpacerItem * ui = nullptr;
            Spacer() { is_terminal = true; is_movable = false; }
            ~Spacer() override { QObject::disconnect(nativeDestroyed); }
            void update(Node * previous) override;
            void * outer() override { return ui; }
            void releaseRetiredRefs() override { release(); Node::releaseRetiredRefs(); }
            void destroy(bool direct) override {
                if(direct && ui) {
                    if(owner) owner->removeItem(ui);
                    delete ui;
                }
                // Indirect teardown leaves the item to its native layout owner.
                release();
            }
        };

        class BaseLayout: public BaseWidget {
            protected:
            void clearNative() override {
                layout = nullptr;
                BaseWidget::clearNative();
            }

            public:
            QBoxLayout * layout = nullptr;

            void update(BaseWidget * prev) override {
                update(static_cast<BaseLayout *>(prev));
                layout->setContentsMargins(0, 0, 0, 0);
                const int padding = dynamic_cast<BaseLayout *>(dom_parent) ? 0 : 11;
                ui->setContentsMargins(padding, padding, padding, padding);
            }
            virtual void update(BaseLayout * prev) = 0;

            void * inner() override { return layout; }
            void addChild(int idx, Node * child) override {
                if(child->is_outoforder) return;
                if(auto spacer = dynamic_cast<Spacer *>(child)) layout->insertSpacerItem(idx, spacer->ui);
                else layout->insertWidget(idx, static_cast<QWidget *>(child->outer()));
            }
            void removeChild(int, Node * child) override {
                if(child->is_outoforder) return;
                if(auto spacer = dynamic_cast<Spacer *>(child)) { if(spacer->ui) layout->removeItem(spacer->ui); }
                else if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
        };
        inline void Spacer::update(Node * previous) {
            auto box = dynamic_cast<BaseLayout *>(dom_parent);
            if(!box || !box->layout) throw std::logic_error("Qt Spacer requires an HBox or VBox parent");
            auto prev = static_cast<Spacer *>(previous);
            ui = prev && prev->ui ? prev->ui : new QSpacerItem(0, 0);
            owner = box->layout;
            const auto direction = owner->direction();
            const bool horizontal = direction == QBoxLayout::LeftToRight || direction == QBoxLayout::RightToLeft;
            ui->changeSize(0, 0, horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred,
                           horizontal ? QSizePolicy::Preferred : QSizePolicy::Expanding);
            owner->invalidate();
            nativeDestroyed = QObject::connect(owner, &QObject::destroyed, [this]() { ui = nullptr; owner = nullptr; });
            Node::update(previous);
        }
    }
}
#endif

#ifndef CPPUI_QT_BASE
#define CPPUI_QT_BASE

#include "../view.hpp"
#include <QWidget>
#include <QBoxLayout>

namespace CPPUI {
    namespace Qt {
        class QtView: public View {
            public:
            ~QtView() override { clearChildren(); }

            void clearChildren() {
                destroyed = true;
                for(Node * child: *children) recur_delete(child, true);
                children->clear();
            }

            // Virtual views own declarations, never a QWidget or QApplication.
            void destroy(bool) override {}
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
                layout->insertWidget(idx, static_cast<QWidget *>(child->outer()));
            }
            void removeChild(int, Node * child) override {
                if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
        };
    }
}
#endif

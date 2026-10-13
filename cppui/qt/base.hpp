#ifndef CPPUI_QT_BASE
#define CPPUI_QT_BASE

#include "../view.hpp"
#include "options.hpp"
#include <QWidget>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QColor>

namespace CPPUI {
    namespace Qt {
        class QtView: public View {
            friend class Views;
            friend class Tabs;
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

        inline QString styleSheet(const Node & node, const QtOptions & options) {
            std::map<std::string, std::string> styles;
            if(node.style_fontsize) styles["font"] = std::to_string(*node.style_fontsize) + "pt";
            if(node.style_fontfamily) styles["font-family"] = *node.style_fontfamily;
            if(node.style_color) styles["color"] = QColor(*node.style_color).name().toStdString();
            if(node.style_bgcolor) styles["background-color"] = QColor(*node.style_bgcolor).name().toStdString();
            if(options.StyleSheet) for(const auto & entry: *options.StyleSheet) styles[entry.first] = entry.second;
            QString sheet;
            for(const auto & entry: styles) sheet += QString::fromStdString(entry.first + ":" + entry.second + ";");
            return sheet;
        }

        class BaseWidget: public Node {
            QMetaObject::Connection nativeDestroyed;
            std::vector<QMetaObject::Connection> connections;
            QtOptions qtParams;
            QSizePolicy defaultPolicy;
            QMargins defaultMargins;
            QLayout::SizeConstraint defaultConstraint = QLayout::SetDefaultConstraint;

            void restoreOptions() {
                if(!ui) return;
                if(layout_width || layout_height || qtParams.HorizontalPolicy || qtParams.VerticalPolicy)
                    ui->setSizePolicy(defaultPolicy);
                if(layout_padding) ui->setContentsMargins(defaultMargins);
                if(qtParams.Style) ui->setStyle(nullptr);
                if(qtParams.SizeConstraint && ui->layout()) ui->layout()->setSizeConstraint(defaultConstraint);
                if(auto hints = dynamic_cast<SizeHints *>(ui)) { hints->widthHint.reset(); hints->heightHint.reset(); }
                ui->setStyleSheet({});
            }
            void applyOptions() {
                defaultPolicy = ui->sizePolicy(); defaultMargins = ui->contentsMargins();
                if(ui->layout()) defaultConstraint = ui->layout()->sizeConstraint();
                if(auto hints = dynamic_cast<SizeHints *>(ui)) {
                    hints->widthHint = layout_width; hints->heightHint = layout_height;
                }
                auto policy = defaultPolicy;
                if(layout_width) policy.setHorizontalPolicy(QSizePolicy::Preferred);
                if(layout_height) policy.setVerticalPolicy(QSizePolicy::Preferred);
                if(qtParams.HorizontalPolicy) policy.setHorizontalPolicy(*qtParams.HorizontalPolicy);
                if(qtParams.VerticalPolicy) policy.setVerticalPolicy(*qtParams.VerticalPolicy);
                ui->setSizePolicy(policy);
                if(layout_padding) {
                    const auto & p = *layout_padding;
                    ui->setContentsMargins(p.left, p.top, p.right, p.bottom);
                }
                if(qtParams.Style) ui->setStyle(*qtParams.Style);
                if(qtParams.SizeConstraint) {
                    if(!ui->layout()) throw std::logic_error("qt(SizeConstraint) requires a layout widget");
                    ui->layout()->setSizeConstraint(*qtParams.SizeConstraint);
                }
                ui->setStyleSheet(styleSheet(*this, qtParams));
                ui->updateGeometry();
            }

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
            Node & qt(const QtOptions & options) override { qtParams.merge(options); return *this; }
            void postUpdate() override;

            void update(Node * previous) override {
                auto prev = static_cast<BaseWidget *>(previous);
                if(prev) { prev->disconnectSignals(); prev->restoreOptions(); }
                update(prev);
                if(ui) {
                    applyOptions();
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
            void postUpdate() override;
            void * outer() override { return ui; }
            void releaseRetiredRefs() override { release(); Node::releaseRetiredRefs(); }
            void destroy(bool direct) override {
                if(ui && (direct || !owner)) {
                    if(owner) owner->removeItem(ui);
                    delete ui;
                }
                // Indirect teardown leaves the item to its native layout owner.
                release();
            }
        };

        class LayoutWidget: public BaseWidget {
            protected:
            void applyPadding() {
                const int padding = dynamic_cast<LayoutWidget *>(dom_parent) ? 0 : 11;
                ui->setContentsMargins(padding, padding, padding, padding);
            }
        };

        class BaseLayout: public LayoutWidget {
            protected:
            void clearNative() override {
                qtlayout = nullptr;
                BaseWidget::clearNative();
            }

            public:
            QBoxLayout * qtlayout = nullptr;

            void update(BaseWidget * prev) override {
                update(static_cast<BaseLayout *>(prev));
                qtlayout->setContentsMargins(0, 0, 0, 0);
                applyPadding();
            }
            virtual void update(BaseLayout * prev) = 0;

            void * inner() override { return qtlayout; }
            void addChild(int idx, Node * child) override {
                if(child->is_outoforder) return;
                if(auto spacer = dynamic_cast<Spacer *>(child)) qtlayout->insertSpacerItem(idx, spacer->ui);
                else qtlayout->insertWidget(idx, static_cast<QWidget *>(child->outer()));
            }
            void removeChild(int, Node * child) override {
                if(child->is_outoforder) return;
                if(auto spacer = dynamic_cast<Spacer *>(child)) { if(spacer->ui) qtlayout->removeItem(spacer->ui); }
                else if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
        };
        inline void updateStretch(Node * node) {
            auto box = dynamic_cast<BaseLayout *>(node->dom_parent);
            if(!box || !box->qtlayout || node->is_outoforder) return;
            const int weight = node->layout_weight.valueOr(dynamic_cast<Spacer *>(node) ? 1 : 0);
            for(int i = 0; i < box->qtlayout->count(); ++i) {
                auto item = box->qtlayout->itemAt(i);
                if((dynamic_cast<Spacer *>(node) && item == node->outer()) || (item->widget() && item->widget() == node->outer())) {
                    box->qtlayout->setStretch(i, weight); break;
                }
            }
        }
        inline void BaseWidget::postUpdate() { updateStretch(this); Node::postUpdate(); }
        inline void Spacer::postUpdate() { updateStretch(this); Node::postUpdate(); }
        inline void Spacer::update(Node * previous) {
            auto box = dynamic_cast<BaseLayout *>(dom_parent);
            if(!dynamic_cast<LayoutWidget *>(dom_parent)) throw std::logic_error("Qt Spacer requires a layout parent");
            auto prev = static_cast<Spacer *>(previous);
            ui = prev && prev->ui ? prev->ui : new QSpacerItem(0, 0);
            owner = box ? box->qtlayout : nullptr;
            const auto direction = owner ? owner->direction() : QBoxLayout::LeftToRight;
            const bool horizontal = direction == QBoxLayout::LeftToRight || direction == QBoxLayout::RightToLeft;
            ui->changeSize(0, 0, !owner || horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred,
                           !owner || !horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred);
            if(owner) {
                owner->invalidate();
                nativeDestroyed = QObject::connect(owner, &QObject::destroyed, [this]() { ui = nullptr; owner = nullptr; });
            }
            Node::update(previous);
        }
    }
}
#endif

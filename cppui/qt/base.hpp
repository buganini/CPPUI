#ifndef CPPUI_QT_BASE
#define CPPUI_QT_BASE

#include "../view.hpp"
#include "adaptive.hpp"
#include "options.hpp"
#include "drop.hpp"
#include <QWidget>
#include <QBoxLayout>
#include <QCoreApplication>
#include <QColor>
#include <QTimer>
#include <algorithm>

namespace CPPUI {
    namespace Qt {
        constexpr char backendName[] = "Qt";

        class QtView: public View {
            friend class Views;
            friend class Tabs;
            QObject callbackContext;
            bool dirty = false, updating = false, scheduled = false, setupDone = false;

            public:
            ~QtView() override { clearChildren(); }

            virtual void setup() {}

            void update(Node * previous) override {
                if(destroyed || retired_by || is_frozen) return;
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
            if(node.style_fontfamily) styles["font-family"] = node.style_fontfamily;
            if(node.style_fontweight) styles["font-weight"] = std::to_string(std::min(900, std::max(100, *node.style_fontweight)));
            if(node.style_color) styles["color"] = QColor(*node.style_color).name().toStdString();
            if(node.style_bgcolor) styles["background-color"] = QColor(*node.style_bgcolor).name().toStdString();
            if(options.StyleSheet) for(const auto & entry: *options.StyleSheet) styles[entry.first] = entry.second;
            QString sheet;
            for(const auto & entry: styles) sheet += QString::fromStdString(entry.first + ":" + entry.second + ";");
            return sheet;
        }

        class BaseWidget: public Node {
            QMetaObject::Connection nativeDestroyed;
            QPointer<DropFilter> dropFilter;
            std::vector<QMetaObject::Connection> connections;
            QtOptions qtParams;
            QSizePolicy defaultPolicy;
            QMargins defaultMargins;
            QLayout::SizeConstraint defaultConstraint = QLayout::SetDefaultConstraint;

            void restoreOptions() {
                if(!ui) return;
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
            int expandX=0, expandY=0;
            int layoutPriority(bool horizontal) const {
                const auto & policy=horizontal ? qtParams.HorizontalPolicy : qtParams.VerticalPolicy;
                if(!policy) return horizontal ? expandX : expandY;
                return *policy==QSizePolicy::Expanding || *policy==QSizePolicy::MinimumExpanding || *policy==QSizePolicy::Ignored ? 3
                    : *policy==QSizePolicy::Preferred || *policy==QSizePolicy::Minimum ? 1 : 0;
            }
            void applyExpansion(bool horizontal,bool main,bool cross) {
                auto policy=ui->sizePolicy();
                policy.setHorizontalPolicy(qtParams.HorizontalPolicy.valueOr((horizontal ? main : cross) ? QSizePolicy::Expanding : QSizePolicy::Preferred));
                policy.setVerticalPolicy(qtParams.VerticalPolicy.valueOr((horizontal ? cross : main) ? QSizePolicy::Expanding : QSizePolicy::Preferred));
                ui->setSizePolicy(policy);
            }
            // This observer is valid only within the explicit native lifetime.
            QWidget * ui = nullptr;

            ~BaseWidget() override { clearRef(); releaseNativeAccess(); }
            CommandPoster commandPoster() override {
                if(!ui) return {};
                auto context=ui;
                return [context](std::function<void()> callback) {
                    QMetaObject::invokeMethod(context,[callback]() {
                        try { callback(); }
                        catch(const std::exception & error) { std::cerr << "CPPUI ref: " << error.what() << '\n'; }
                        catch(...) { std::cerr << "CPPUI ref failed\n"; }
                    },::Qt::QueuedConnection);
                };
            }
            TimerPoster timerPoster() override {
                if(!ui) return {};
                auto context=ui;
                return [context](int delay,Callback callback) {
                    QTimer::singleShot(delay,::Qt::PreciseTimer,context,[callback]() {
                        try { callback(); }
                        catch(const std::exception & error) { std::cerr<<"CPPUI posted callback: "<<error.what()<<'\n'; }
                        catch(...) { std::cerr<<"CPPUI posted callback failed\n"; }
                    });
                };
            }
            Node & qt(const QtOptions & options) override { qtParams.merge(options); return *this; }
            std::unique_ptr<detail::SizeObserver> observeSize() override {
                return ui ? std::unique_ptr<detail::SizeObserver>(new SizeObserver(ui,sizeState)) : nullptr;
            }
            void postUpdate() override;

            void update(Node * previous) override {
                auto prev = static_cast<BaseWidget *>(previous);
                if(prev) { prev->disconnectSignals(); prev->restoreOptions(); }
                update(prev);
                if(prev) dropFilter=prev->dropFilter;
                if(ui) {
                    if(dropHandlers.entered || dropHandlers.dropped) {
                        if(!dropFilter) dropFilter=new DropFilter(ui);
                        dropFilter->handlers=dropHandlers; dropFilter->enable(true);
                    } else if(dropFilter) { dropFilter->handlers={}; dropFilter->enable(false); }
                    applyOptions();
                    if(supportsVisibility() && (!is_visible || (prev && !prev->is_visible))) ui->setVisible(is_visible);
                    nativeDestroyed = QObject::connect(ui, &QObject::destroyed, [this]() {
                        clearRef(); clearNative();
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
                if(dropFilter) dropFilter->handlers={};
                dropHandlers={};
                clearRef();
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
            Spacer() { is_terminal = true; is_movable = false; layout_weight=1; }
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
            std::vector<Node *> mountedChildren;
            protected:
            void clearNative() override {
                qtlayout = nullptr;
                mountedChildren.clear();
                BaseWidget::clearNative();
            }

            public:
            QBoxLayout * qtlayout = nullptr;

            void update(BaseWidget * prev) override {
                if(prev && prev->ui) mountedChildren=std::move(static_cast<BaseLayout *>(prev)->mountedChildren);
                update(static_cast<BaseLayout *>(prev));
                qtlayout->setContentsMargins(0, 0, 0, 0);
                applyPadding();
            }
            virtual void update(BaseLayout * prev) = 0;
            bool reflowChildren() const override { return true; }
            void rebindChild(Node * previous,Node * current) override {
                for(auto & child:mountedChildren) if(child==previous) child=current;
            }
            void postSync() override {
                if(!qtlayout) return;
                const auto direction=qtlayout->direction();
                const bool horizontal=direction==QBoxLayout::LeftToRight || direction==QBoxLayout::RightToLeft;
                auto mounted=[](Node * child) {
                    if(auto spacer=dynamic_cast<Spacer *>(child)) return spacer->ui!=nullptr;
                    auto widget=dynamic_cast<BaseWidget *>(child);
                    return widget && widget->ui;
                };
                auto priority=[horizontal](Node * child) {
                    if(child->layout_weight) return *child->layout_weight>0 ? 4 : 0;
                    auto widget=dynamic_cast<BaseWidget *>(child);
                    return widget ? widget->layoutPriority(horizontal) : 0;
                };
                int highest=0;
                for(auto child:mountedChildren) if(mounted(child) && child->layoutVisible()) highest=std::max(highest,priority(child));
                const auto leading=horizontal ? ::Qt::AlignLeft : ::Qt::AlignTop;
                const auto crossAlignment=horizontal ? ::Qt::AlignVCenter : ::Qt::AlignLeft;
                qtlayout->setAlignment(highest ? ::Qt::Alignment() : leading);
                for(std::size_t i=0;i<mountedChildren.size();++i) {
                    auto child=mountedChildren[i];
                    if(!mounted(child)) continue;
                    auto spacer=dynamic_cast<Spacer *>(child);
                    auto widget=dynamic_cast<BaseWidget *>(child);
                    int index=static_cast<int>(i);
                    auto item=qtlayout->itemAt(index);
                    // External QWidget deletion can remove a native item before
                    // its declaration is reconciled. Keep independent views safe.
                    if(spacer && item!=spacer->ui) index=qtlayout->indexOf(spacer->ui);
                    if(widget && (!item || item->widget()!=widget->ui)) index=qtlayout->indexOf(widget->ui);
                    if(index<0) continue;
                    const bool expands=child->layoutVisible() && highest>0 && priority(child)==highest;
                    qtlayout->setStretch(index,expands ? child->layout_weight.valueOr(1) : 0);
                    if(spacer) {
                        const auto main=expands ? QSizePolicy::Expanding : QSizePolicy::Fixed;
                        spacer->ui->changeSize(0,0,horizontal ? main : QSizePolicy::Minimum,horizontal ? QSizePolicy::Minimum : main);
                    } else if(widget) {
                        const bool cross=widget->layoutPriority(!horizontal)>0;
                        qtlayout->itemAt(index)->setAlignment((expands ? ::Qt::Alignment() : leading) |
                                                                 (cross ? ::Qt::Alignment() : crossAlignment));
                        widget->applyExpansion(horizontal,expands,cross);
                    }
                }
                qtlayout->invalidate();
            }
            void postUpdate() override { postSync();BaseWidget::postUpdate(); }

            void * inner() override { return qtlayout; }
            void addChild(int idx, Node * child) override {
                if(child->is_outoforder) return;
                mountedChildren.insert(mountedChildren.begin()+idx,child);
                if(auto spacer = dynamic_cast<Spacer *>(child)) qtlayout->insertSpacerItem(idx, spacer->ui);
                else qtlayout->insertWidget(idx, static_cast<QWidget *>(child->outer()));
            }
            void removeChild(int index, Node * child) override {
                if(child->is_outoforder) return;
                mountedChildren.erase(mountedChildren.begin()+index);
                if(auto spacer = dynamic_cast<Spacer *>(child)) { if(spacer->ui) qtlayout->removeItem(spacer->ui); }
                else if(auto widget = static_cast<QWidget *>(child->outer())) widget->setParent(nullptr);
            }
        };
        inline void BaseWidget::postUpdate() { if(ui && !layoutVisible()) ui->hide(); Node::postUpdate(); }
        inline void Spacer::postUpdate() { Node::postUpdate(); }
        inline void Spacer::update(Node * previous) {
            auto box = dynamic_cast<BaseLayout *>(dom_parent);
            if(!dynamic_cast<LayoutWidget *>(dom_parent)) throw std::logic_error("Qt Spacer requires a layout parent");
            auto prev = static_cast<Spacer *>(previous);
            ui = prev && prev->ui ? prev->ui : new QSpacerItem(0, 0);
            owner = box ? box->qtlayout : nullptr;
            const auto direction = owner ? owner->direction() : QBoxLayout::LeftToRight;
            const bool horizontal = direction == QBoxLayout::LeftToRight || direction == QBoxLayout::RightToLeft;
            ui->changeSize(0, 0, !layoutVisible() ? QSizePolicy::Fixed : (!owner || horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred),
                           !layoutVisible() ? QSizePolicy::Fixed : (!owner || !horizontal ? QSizePolicy::Expanding : QSizePolicy::Preferred));
            if(owner) {
                owner->invalidate();
                nativeDestroyed = QObject::connect(owner, &QObject::destroyed, [this]() { ui = nullptr; owner = nullptr; });
            }
            Node::update(previous);
        }
    }
}
#endif

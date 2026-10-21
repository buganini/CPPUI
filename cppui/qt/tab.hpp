#ifndef CPPUI_QT_TAB
#define CPPUI_QT_TAB

#include "layout.hpp"
#include "../binding.hpp"
#include "../side.hpp"
#include <QTabWidget>
#include <QPointer>
#include <memory>
#include <stdexcept>
#include <string>

namespace CPPUI {
    namespace Qt {
        class Tab: public VBox {
            public:
            bool supportsVisibility() const override { return false; }
            std::string label;
            explicit Tab(const std::string & label): label(label) { layout_padding = Insets(0); }
        };

        class Tabs: public BaseWidget {
            Optional<Binding<OptionalText>> model;
            std::shared_ptr<OptionalText> internal{new OptionalText()};
            OptionalText selected;
            bool initialized = false, syncing = true;
            int applied = -1;
            static QTabWidget::TabPosition nativePosition(Side position) {
                if(position == Side::TOP) return QTabWidget::North;
                if(position == Side::BOTTOM) return QTabWidget::South;
                if(position == Side::RIGHT) return QTabWidget::East;
                if(position == Side::LEFT) return QTabWidget::West;
                throw std::invalid_argument("Unknown tab position");
            }
            static std::string key(const Node * node) {
                const auto tab = static_cast<const Tab *>(node);
                return tab->has_id && !tab->_id.empty() ? tab->_id : tab->label;
            }
            public:
            Side tabposition;

            explicit Tabs(Side tabposition = Side::TOP): tabposition(tabposition) {
                expandX=expandY=2;
                nativePosition(tabposition);
                if(!dynamic_cast<QtView *>(cppui_root)) throw std::logic_error("Tabs requires a Qt Application or QtView");
            }
            Tabs(Side tabposition, Binding<OptionalText> model): Tabs(tabposition) {
                this->model = model;
                selected = model.get();
            }
            void prepare() override {
                for(auto child: *children) if(!dynamic_cast<Tab *>(child))
                    throw std::invalid_argument("Tabs children must be Tab declarations");
                Node::prepare();
            }
            void update(BaseWidget * previous) override {
                auto prev = static_cast<Tabs *>(previous);
                if(!model) {
                    if(prev && !prev->model) { internal = prev->internal; initialized = prev->initialized; }
                    selected = *internal;
                }
                ui = prev && prev->ui ? prev->ui : new Widget<QTabWidget>();
                auto tabs = static_cast<QTabWidget *>(ui);
                tabs->setTabPosition(nativePosition(tabposition));
                connectSignal(QObject::connect(tabs, &QTabWidget::currentChanged, tabs, [this](int index) {
                    if(destroyed || syncing || index == applied || index < 0 || index >= static_cast<int>(children->size())) return;
                    const auto selectedKey = key(children->at(index));
                    // select snapshots everything needed before invoking a setter.
                    try { select(selectedKey); }
                    catch(const std::exception & error) { std::cerr << "CPPUI Tabs selection: " << error.what() << '\n'; }
                    catch(...) { std::cerr << "CPPUI Tabs selection: unknown exception\n"; }
                }));
            }
            void preSync() override { syncing = true; }
            void addChild(int idx, Node * child) override {
                auto tab = static_cast<Tab *>(child);
                static_cast<QTabWidget *>(ui)->insertTab(idx, static_cast<QWidget *>(tab->outer()), QString::fromStdString(tab->label));
            }
            void removeChild(int idx, Node *) override { static_cast<QTabWidget *>(ui)->removeTab(idx); }
            void postSync() override {
                auto tabs = static_cast<QTabWidget *>(ui);
                int index = children->empty() ? -1 : 0;
                bool matched = false;
                for(std::size_t i = 0; i < children->size(); ++i) {
                    tabs->setTabText(static_cast<int>(i), QString::fromStdString(static_cast<Tab *>(children->at(i))->label));
                    if(!matched && selected && key(children->at(i)) == *selected) {
                        index = static_cast<int>(i); matched = true;
                    }
                }
                if(!model && !initialized && index >= 0) {
                    if(!selected) *internal = key(children->at(index));
                    initialized = true;
                }
                tabs->setCurrentIndex(index);
                applied = index;
                syncing = false;
            }
            bool refSelection() const override { return true; }
            bool refEmptySelection() const override { return true; }
            void refCommand(RefCommand, const OptionalText & key, bool) override { select(key); }
            void select(OptionalText key = {}) {
                if(destroyed || retired_by || !ui) return;
                auto binding = model;
                auto owned = internal;
                auto root = static_cast<QtView *>(cppui_root);
                QPointer<QWidget> alive(ui);
                QPointer<QObject> viewAlive(&root->callbackContext);
                if(binding) binding->set(key);
                else *owned = key;
                if(alive && viewAlive) root->redraw();
            }
            void select(const std::string & key) { select(OptionalText(key)); }
            void select(const char * key) { select(key ? OptionalText(std::string(key)) : OptionalText()); }
        };
    }
    inline Qt::Tabs & Tabs(Side tabposition = Side::TOP) { return *new Qt::Tabs(tabposition); }
    inline Qt::Tabs & Tabs(Side tabposition, Binding<OptionalText> model) { return *new Qt::Tabs(tabposition, model); }
    inline Qt::Tabs & Tabs(Binding<OptionalText> model) { return *new Qt::Tabs(Side::TOP, model); }
    inline Qt::Tab & Tab(const std::string & label) { return *new Qt::Tab(label); }
}
#endif

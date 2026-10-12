#ifndef CPPUI_QT_TAB
#define CPPUI_QT_TAB

#include "base.hpp"
#include <QTabWidget>
#include <stdexcept>
#include <string>

namespace CPPUI {
    namespace Qt {
        class Tab: public Node {
            public:
            std::string label;

            explicit Tab(const std::string & label): label(label) { is_virtual = true; }
            void destroy(bool) override {}
        };

        class Tabs: public BaseWidget {
            public:
            // PUI stores this option but does not apply it to the native widget.
            std::string tabposition;

            explicit Tabs(const std::string & tabposition = "n"): tabposition(tabposition) {}

            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new QTabWidget();
            }
            void addChild(int idx, Node * child) override {
                if(child->is_outoforder) return;
                Node * parent = child->parent;
                while(parent && parent != this) {
                    if(auto tab = dynamic_cast<Tab *>(parent)) {
                        static_cast<QTabWidget *>(ui)->insertTab(idx,
                            static_cast<QWidget *>(child->outer()), QString::fromStdString(tab->label));
                        return;
                    }
                    if(parent == parent->parent) break;
                    parent = parent->parent;
                }
                throw std::logic_error("Tabs pages must have an ancestor Tab");
            }
            void removeChild(int idx, Node * child) override {
                if(child->is_outoforder) return;
                static_cast<QTabWidget *>(ui)->removeTab(idx);
            }
            void postSync() override {
                // Preserve PUI's declaration-index label update, including its
                // mismatch when an empty/multi-child Tab changes page counts.
                int index = 0;
                for(auto child: *children) {
                    auto tab = dynamic_cast<Tab *>(child);
                    if(!tab) throw std::logic_error("Tabs declarations must be Tab nodes");
                    static_cast<QTabWidget *>(ui)->setTabText(index++, QString::fromStdString(tab->label));
                }
            }
        };
    }
    inline Qt::Tabs & Tabs(const std::string & tabposition = "n") { return *new Qt::Tabs(tabposition); }
    inline Qt::Tab & Tab(const std::string & label) { return *new Qt::Tab(label); }
}
#endif

#ifndef CPPUI_QT_VIEWS
#define CPPUI_QT_VIEWS

#include "layout.hpp"
#include "../binding.hpp"
#include <QStackedWidget>
#include <QPointer>
#include <memory>
#include <set>

namespace CPPUI { namespace Qt {
    class Views: public BaseWidget {
        Optional<Binding<OptionalText>> model;
        std::shared_ptr<OptionalText> internal{new OptionalText()};
        OptionalText selected;
        bool prepared = false;
        static void validate(const OptionalText & key) {
            if(key && key->empty()) throw std::invalid_argument("Views selection must be a non-empty string or empty OptionalText");
        }
        public:
        Views() {
            if(!dynamic_cast<QtView *>(cppui_root)) throw std::logic_error("Views requires a Qt Application or QtView");
        }
        explicit Views(Binding<OptionalText> model): Views() {
            this->model = model;
            selected = model.get();
            validate(selected);
        }
        void prepare() override {
            if(!prepared) {
                std::set<std::string> keys;
                for(auto child: *children) {
                    if(!child->has_id || child->_id.empty() || !keys.insert(child->_id).second)
                        throw std::invalid_argument("Each Views child needs a unique, non-empty string ID");
                    if(child->is_outoforder) throw std::invalid_argument("Views pages must be ordinary widgets, layouts or views");
                }
                const auto count = children->size();
                std::vector<Node *> pages;
                pages.reserve(count);
                {
                    auto rootScope = cppui_root->begin();
                    auto scope = begin();
                    // Keep originals and new empty pages owned by this node
                    // until all potentially throwing allocations have finished.
                    for(std::size_t i = 0; i < count; ++i) {
                        auto page = new VBox();
                        page->id(children->at(i)->_id).layout({}, {}, {}, Insets(0));
                        page->children->reserve(1);
                        pages.push_back(page);
                    }
                }
                for(std::size_t i = 0; i < count; ++i) {
                    auto child = children->at(i);
                    child->parent = pages[i];
                    pages[i]->children->push_back(child);
                }
                children->swap(pages);
                prepared = true;
            }
            Node::prepare();
        }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Views *>(previous);
            if(!model) {
                if(prev && !prev->model) internal = prev->internal;
                selected = *internal;
            }
            if(prev && prev->ui) ui = prev->ui;
            else {
                auto stack = new Widget<QStackedWidget>();
                stack->addWidget(new QWidget()); // Index zero represents no selection.
                ui = stack;
            }
        }
        void addChild(int index, Node * child) override {
            static_cast<QStackedWidget *>(ui)->insertWidget(index + 1, static_cast<QWidget *>(child->outer()));
        }
        void removeChild(int, Node * child) override {
            if(auto page = static_cast<QWidget *>(child->outer())) {
                static_cast<QStackedWidget *>(ui)->removeWidget(page);
                page->setParent(nullptr);
            }
        }
        void postSync() override {
            int index = 0;
            if(selected) for(std::size_t i = 0; i < children->size(); ++i) {
                if(children->at(i)->_id == *selected) { index = static_cast<int>(i) + 1; break; }
            }
            if(ui) static_cast<QStackedWidget *>(ui)->setCurrentIndex(index);
        }
        void select(OptionalText key = {}) {
            validate(key);
            if(destroyed || retired_by || !ui) return;
            auto binding = model;
            auto owned = internal;
            auto root = static_cast<QtView *>(cppui_root);
            QPointer<QWidget> alive(ui);
            QPointer<QObject> viewAlive(&root->callbackContext);
            // Setters can redraw and remove this declaration. Keep only owned
            // snapshots and the guarded native lifetime across application code.
            if(binding) binding->set(key);
            else *owned = key;
            if(alive && viewAlive) root->redraw();
        }
        void select(const std::string & key) { select(OptionalText(key)); }
        void select(const char * key) { select(key ? OptionalText(std::string(key)) : OptionalText()); }
    };
}}
namespace CPPUI {
    inline Qt::Views & Views() { return *new Qt::Views(); }
    inline Qt::Views & Views(Binding<OptionalText> model) { return *new Qt::Views(model); }
}
#endif

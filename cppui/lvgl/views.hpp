#ifndef CPPUI_LVGL_VIEWS
#define CPPUI_LVGL_VIEWS

#include "layout.hpp"
#include "../binding.hpp"
#include <memory>
#include <set>

namespace CPPUI { namespace LVGL {
    class Views: public Node {
        struct Selection {
            OptionalText key;
            Views * current = nullptr;
        };
        Optional<Binding<OptionalText>> model;
        std::shared_ptr<Selection> selection{new Selection()};
        OptionalText selected;
        bool prepared = false;
        static void validate(const OptionalText & key) {
            if(key && key->empty()) throw std::invalid_argument("Views selection must be a non-empty string or empty OptionalText");
        }
        public:
        Views() {
            if(!dynamic_cast<View *>(cppui_root)) throw std::logic_error("Views requires an LVGL Application or View");
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
                std::vector<::CPPUI::Node *> pages;
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
        ~Views() override { retire(); }
        void retire() { if(selection->current == this) selection->current = nullptr; }
        void destroy(bool direct) override { retire(); Node::destroy(direct); }
        void update(Node * previous) override {
            auto prev = static_cast<Views *>(previous);
            if(prev) selection = prev->selection;
            selection->current = this;
            if(!model) {
                if(prev && prev->model) selection->key.reset();
                selected = selection->key;
            }
            ui = prev && prev->ui ? prev->ui : lv_obj_create(static_cast<lv_obj_t *>(parent->inner()));
            auto object = static_cast<lv_obj_t *>(ui);
            lv_obj_set_style_bg_opa(object, LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(object, 0, 0);
            lv_obj_set_style_radius(object, 0, 0);
            lv_obj_set_style_pad_all(object, 0, 0);
            lv_obj_set_scrollable(object, false);
            lv_obj_set_size(object, lv_pct(100), lv_pct(100));
        }
        void layoutChild(Node * child) override {
            if(child->ui) {
                auto page = static_cast<lv_obj_t *>(child->ui);
                lv_obj_set_pos(page, 0, 0);
                lv_obj_set_size(page, lv_pct(100), lv_pct(100));
            }
        }
        void postSync() override {
            for(auto child: *children) {
                auto page = static_cast<lv_obj_t *>(child->ui);
                if(page) lv_obj_set_hidden(page, !selected || child->_id != *selected);
            }
        }
        bool refSelection() const override { return true; }
        void refCommand(RefCommand, const OptionalText & key, bool) override { select(key); }
        void select(OptionalText key = {}) {
            validate(key);
            if(destroyed || retired_by || !ui) return;
            auto binding = model;
            auto state = selection;
            // LVGL state writes redraw synchronously. Keep the selection's
            // dispatch state alive while setters replace/remove this node.
            if(binding) binding->set(key);
            else state->key = key;
            auto current = state->current;
            if(current && current->ui && !current->destroyed) current->cppui_root->redraw();
        }
        void select(const std::string & key) { select(OptionalText(key)); }
        void select(const char * key) { select(key ? OptionalText(std::string(key)) : OptionalText()); }
    };
}}
namespace CPPUI {
    inline LVGL::Views & Views() { return *new LVGL::Views(); }
    inline LVGL::Views & Views(Binding<OptionalText> model) { return *new LVGL::Views(model); }
}
#endif

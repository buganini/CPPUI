#ifndef CPPUI_LVGL_TAB
#define CPPUI_LVGL_TAB

#include "base.hpp"
#include <stdexcept>
#include <string>

namespace CPPUI {
    namespace LVGL {
        class Tab: public Node {
            public:
            std::string label;
            explicit Tab(const std::string & label): label(label) { is_virtual = true; }
            void update(Node *) override {}
            void destroy(bool) override {}
        };
        class Tabs: public Node {
            // A selected page temporarily removed for a keyed move. Observe its
            // child, so actual removal cannot leave a dangling selection bookmark.
            lv_obj_t * movingSelection = nullptr;
            static void selectionDeleted(lv_event_t * event) {
                static_cast<Tabs *>(lv_event_get_user_data(event))->movingSelection = nullptr;
            }
            void forgetSelection() {
                if(movingSelection) lv_obj_remove_event_cb_with_user_data(movingSelection, selectionDeleted, this);
                movingSelection = nullptr;
            }
            lv_obj_t * selected() const {
                if(!ui) return nullptr;
                auto object = static_cast<lv_obj_t *>(ui);
                const auto index = lv_tabview_get_tab_active(object);
                if(index >= lv_tabview_get_tab_count(object)) return nullptr;
                auto page = lv_obj_get_child(lv_tabview_get_content(object), index);
                return page ? lv_obj_get_child(page, 0) : nullptr;
            }
            void restoreSelection(lv_obj_t * child, uint32_t fallback) {
                auto object = static_cast<lv_obj_t *>(ui);
                const auto count = lv_tabview_get_tab_count(object);
                auto content = lv_tabview_get_content(object);
                for(uint32_t i = 0; child && i < count; ++i) {
                    if(lv_obj_get_child(lv_obj_get_child(content, i), 0) == child) {
                        lv_tabview_set_active(object, i, LV_ANIM_OFF);
                        return;
                    }
                }
                lv_tabview_set_active(object, count ? std::min(fallback, count - 1) : 0, LV_ANIM_OFF);
            }
            public:
            // The pinned PUI reference stores this option without applying it.
            std::string tabposition;
            explicit Tabs(const std::string & tabposition = "n"): tabposition(tabposition) {}
            ~Tabs() override { forgetSelection(); }
            void update(Node * prev) override {
                ui = prev && prev->ui ? prev->ui : lv_tabview_create(static_cast<lv_obj_t *>(parent->inner()));
            }
            void destroy(bool direct) override {
                forgetSelection();
                Node::destroy(direct);
            }
            void addChild(int idx, Node * child) override {
                if(child->is_outoforder || !ui || !child->ui) return;
                auto ancestor = child->parent;
                Tab * tab = nullptr;
                while(ancestor && ancestor != this) {
                    tab = dynamic_cast<Tab *>(ancestor);
                    if(tab) break;
                    if(ancestor == ancestor->parent) break;
                    ancestor = ancestor->parent;
                }
                if(!tab) throw std::logic_error("Tabs pages must have an ancestor Tab");
                auto object = static_cast<lv_obj_t *>(ui);
                auto active = selected();
                const auto index = lv_tabview_get_tab_active(object);
                auto page = lv_tabview_add_tab(object, tab->label.c_str());
                auto button = lv_tabview_get_tab_button(object, -1);
                lv_obj_set_style_pad_all(page, 0, 0);
                lv_obj_set_style_border_width(page, 0, 0);
                lv_obj_set_style_radius(page, 0, 0);
                auto content = static_cast<lv_obj_t *>(child->outer());
                lv_obj_set_parent(content, page);
                lv_obj_set_ignore_layout(content, false);
                lv_obj_set_hidden(content, false);
                lv_obj_set_size(content, lv_pct(100), lv_pct(100));
                lv_obj_move_to_index(page, idx);
                lv_obj_move_to_index(button, idx);
                if(movingSelection == content) {
                    active = movingSelection;
                    forgetSelection();
                }
                restoreSelection(active, index);
            }
            void removeChild(int idx, Node * child) override {
                if(child->is_outoforder || !ui) return;
                auto object = static_cast<lv_obj_t *>(ui);
                auto active = selected();
                const auto index = lv_tabview_get_tab_active(object);
                auto page = lv_obj_get_child(lv_tabview_get_content(object), idx);
                auto button = lv_tabview_get_tab_button(object, idx);
                if(!page || !button) return;
                if(child->ui) {
                    auto content = static_cast<lv_obj_t *>(child->outer());
                    if(active == content) {
                        forgetSelection();
                        movingSelection = content;
                        lv_obj_add_event_cb(content, selectionDeleted, LV_EVENT_DELETE, this);
                    }
                    // Core reconciliation decides whether this is a move or a
                    // removal. Keep the child alive until it makes that decision.
                    lv_obj_set_parent(content, object);
                    lv_obj_set_ignore_layout(content, true);
                    lv_obj_set_hidden(content, true);
                }
                lv_obj_delete(button);
                lv_obj_delete(page);
                restoreSelection(active, index);
            }
            void postSync() override {
                if(!ui) return;
                auto object = static_cast<lv_obj_t *>(ui);
                const auto count = lv_tabview_get_tab_count(object);
                uint32_t index = 0;
                // Preserve the pinned reference's declaration-index label quirk.
                for(auto child: *children) {
                    auto tab = dynamic_cast<Tab *>(child);
                    if(!tab) throw std::logic_error("Tabs declarations must be Tab nodes");
                    if(index < count) lv_tabview_set_tab_text(object, index, tab->label.c_str());
                    ++index;
                }
            }
        };
    }
    inline LVGL::Tabs & Tabs(const std::string & tabposition = "n") { return *new LVGL::Tabs(tabposition); }
    inline LVGL::Tab & Tab(const std::string & label) { return *new LVGL::Tab(label); }
}
#endif

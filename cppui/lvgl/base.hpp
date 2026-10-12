#include "../view.hpp"

#include <lvgl.h>
#include <cstdio>

#ifndef CPPUI_LVGL_BASE
#define CPPUI_LVGL_BASE

namespace CPPUI {
    namespace LVGL {
        class View: public ::CPPUI::View {
            public:
            ~View() override {
                destroyed = true;
                for(auto child: *children) recur_delete(child, true);
                children->clear();
            }
            void destroy(bool) override {
                // Virtual views own declarations, not a native object.
                ui = nullptr;
            }
         };

        class Node: public ::CPPUI::Node {
            lv_obj_t * observed = nullptr;
            static void event(lv_event_t * event) {
                auto node = static_cast<Node *>(lv_event_get_user_data(event));
                if(lv_event_get_code(event) == LV_EVENT_DELETE) {
                    node->ui = nullptr;
                    node->observed = nullptr;
                } else if(lv_event_get_code(event) == LV_EVENT_CLICKED) {
                    try {
                        // User code may synchronously replace or delete this declaration.
                        auto invoke = node->onClicked;
                        if(invoke) invoke();
                    } catch(const std::exception & error) {
                        std::fprintf(stderr, "CPPUI LVGL click failed: %s\n", error.what());
                    } catch(...) {
                        std::fprintf(stderr, "CPPUI LVGL click failed: unknown exception\n");
                    }
                }
            }
            void unobserve() {
                if(observed) lv_obj_remove_event_cb_with_user_data(observed, event, this);
                observed = nullptr;
            }
            protected:
            bool clickable = false;
            public:
            ~Node() override { unobserve(); }
            void update(::CPPUI::Node * prev) override {
                update((Node *) prev);
                if(prev) static_cast<Node *>(prev)->unobserve();
                observed = static_cast<lv_obj_t *>(ui);
                if(observed) {
                    lv_obj_add_event_cb(observed, event, clickable ? LV_EVENT_ALL : LV_EVENT_DELETE, this);
                    if(clickable) lv_obj_set_clickable(observed, true);
                }
                ::CPPUI::Node::update(prev);
            }

            virtual void update(Node * prev) = 0;
            virtual void addChild(int idx, Node * child) {
                if(child->ui) lv_obj_move_to_index(static_cast<lv_obj_t *>(child->outer()), idx);
            }

            virtual void removeChild(int idx, Node * child) {
                if(child->ui) lv_obj_move_to_index(static_cast<lv_obj_t *>(child->outer()), -1);
            }

            void addChild(int idx, ::CPPUI::Node *child) {
                addChild(idx, (Node *) child);
            }

            void removeChild(int idx, ::CPPUI::Node *child) {
                removeChild(idx, (Node *) child);
            }

            void destroy(bool direct) {
                unobserve();
                if(direct && ui){
                    lv_obj_delete((lv_obj_t *)ui);
                }
                ui = nullptr;
            }
         };
    }
}

#endif

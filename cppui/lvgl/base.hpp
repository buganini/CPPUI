#include "../view.hpp"

#include <lvgl.h>
#include <cstdio>
#include <memory>

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
            friend class Tabs;
            struct Observer {
                lv_obj_t * object;
                Node * owner;
                Observer ** bookmark = nullptr;
                void invalidate() {
                    object = nullptr;
                    owner = nullptr;
                    if(bookmark) { *bookmark = nullptr; bookmark = nullptr; }
                }
                Observer(lv_obj_t * object, Node * owner): object(object), owner(owner) {}
            };
            std::unique_ptr<Observer> observed;
            static void event(lv_event_t * event) {
                auto observer = static_cast<Observer *>(lv_event_get_user_data(event));
                auto node = observer->owner;
                if(lv_event_get_code(event) == LV_EVENT_DELETE) {
                    node->ui = nullptr;
                    observer->invalidate();
                } else {
                    try { node->nativeEvent(event); }
                    catch(const std::exception & error) {
                        std::fprintf(stderr, "CPPUI LVGL event failed: %s\n", error.what());
                    } catch(...) {
                        std::fprintf(stderr, "CPPUI LVGL event failed: unknown exception\n");
                    }
                }
                // User code may have deleted both the declaration and observer.
            }
            void unobserve() {
                if(observed && observed->object)
                    lv_obj_remove_event_cb_with_user_data(observed->object, event, observed.get());
                if(observed) observed->invalidate();
                observed.reset();
            }
            protected:
            bool clickable = false;
            virtual void nativeEvent(lv_event_t * event) {
                if(clickable && lv_event_get_code(event) == LV_EVENT_CLICKED) {
                    auto invoke = onClicked;
                    if(invoke) invoke();
                }
            }
            public:
            ~Node() override { unobserve(); }
            void update(::CPPUI::Node * previous) override {
                auto prev = static_cast<Node *>(previous);
                update(prev);
                auto object = static_cast<lv_obj_t *>(ui);
                if(prev && prev->observed && prev->observed->object == object) {
                    // Keep the native event list unchanged during a callback that
                    // redraws and then removes itself (LVGL retains its storage).
                    observed = std::move(prev->observed);
                    observed->owner = this;
                } else {
                    if(prev) prev->unobserve();
                    if(object) {
                        observed.reset(new Observer(object, this));
                        lv_obj_add_event_cb(object, event, LV_EVENT_ALL, observed.get());
                    }
                }
                if(object && clickable) lv_obj_set_clickable(object, true);
                ::CPPUI::Node::update(previous);
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

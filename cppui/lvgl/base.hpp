#include "../view.hpp"

#include <lvgl.h>

#ifndef CPPUI_LVGL_BASE
#define CPPUI_LVGL_BASE

namespace CPPUI {
    namespace LVGL {
        class View: public ::CPPUI::View {
            public:
            void destroy(bool direct) {
                if(direct && ui){
                    lv_obj_delete((lv_obj_t *)ui);
                }
            }
         };

        class Node: public ::CPPUI::Node {
            public:
            void update(::CPPUI::Node * prev) {
                update((Node *) prev);
            }

            virtual void update(Node * prev) = 0;
            virtual void addChild(int idx, Node * child) {
                lv_obj_move_to_index((lv_obj_t *)child->outer(), idx);
            }

            virtual void removeChild(int idx, Node * child) {
                lv_obj_move_to_index((lv_obj_t *)child->outer(), -1);
            }

            void addChild(int idx, ::CPPUI::Node *child) {
                addChild(idx, (Node *) child);
            }

            void removeChild(int idx, ::CPPUI::Node *child) {
                removeChild(idx, (Node *) child);
            }

            void destroy(bool direct) {
                if(direct && ui){
                    lv_obj_delete((lv_obj_t *)ui);
                }
            }
         };
    }
}

#endif
#include "../view.hpp"

#ifndef CPPUI_WX_BASE
#define CPPUI_WX_BASE

#include <wx/wx.h>

namespace CPPUI {
    namespace Wx {
        class View: public ::CPPUI::View {
            public:

            void destroy(bool direct) {
                if(direct && ui){
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
            }

            virtual void removeChild(int idx, Node * child) {
            }

            void addChild(int idx, ::CPPUI::Node *child) {
                addChild(idx, (Node *) child);
            }

            void removeChild(int idx, ::CPPUI::Node *child) {
                removeChild(idx, (Node *) child);
            }
         };
    }
}

#endif
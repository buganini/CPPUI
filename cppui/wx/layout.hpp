#include "base.hpp"

#ifndef CPPUI_WX_LAYOUT
#define CPPUI_WX_LAYOUT

namespace CPPUI {
    namespace Wx {
        class Layout: public Node {
            void addChild(int idx, Node * child) {
                if(child->classtype==2){
                    ((wxSizer *)ui)->Add((wxSizer *)child->outer());
                } else {
                    ((wxSizer *)ui)->Add((wxControl *)(child->outer()));
                }
            }
            void removeChild(int idx, Node * child) {
                if(child->classtype==2){
                    ((wxSizer *)ui)->Detach((wxSizer *)child->outer());
                } else {
                    ((wxSizer *)ui)->Detach((wxControl *)(child->outer()));
                }
            }

            void destroy(bool direct) {
                if(direct && ui){
                    delete (wxBoxSizer *)ui;
                }
            }
        };
        class HBox: public Layout {
            public:
            void update(Node * prev) {
                classtype = 2;
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new wxBoxSizer(wxHORIZONTAL);
                }
            }
       };

        class VBox: public Layout {
            public:
            void update(Node * prev) {
                classtype = 2;
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new wxBoxSizer(wxVERTICAL);
                }
            }
        };

    }
    Wx::HBox & HBox() {
        return *(new Wx::HBox());
    }
    Wx::VBox & VBox() {
        return *(new Wx::VBox());
    }
}

#endif
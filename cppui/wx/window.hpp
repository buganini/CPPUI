#include "base.hpp"
#include "layout.hpp"

#include <iostream>

#include <wx/wx.h>

namespace CPPUI {
    namespace Wx {
        class Window: public Node {
            public:
            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                } else {
                    ui = new wxFrame(nullptr, wxID_ANY, "lala");
                }
            }

            void addChild(int idx, Node * child) {
            }

            void removeChild(int idx, Node * child) {
            }

            void postSync(void) {
                if(children->size()) {
                    ::CPPUI::Node * child = children->front();
                    if(classtype!=2 && child->classtype==2) {
                        ((wxFrame *)ui)->SetSizer((wxSizer *)child->outer());
                        ((wxFrame *)ui)->Layout();
                    }
                }
            }

            void destroy(bool direct) {
                if(direct && ui){
                    delete (wxFrame *)ui;
                }
            }

       };
    }
    Wx::Window & Window() {
        return *(new Wx::Window());
    }
}

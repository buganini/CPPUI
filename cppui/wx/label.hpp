#include "base.hpp"

#include <wx/wx.h>

#ifndef CPPUI_WX_LABEL
#define CPPUI_WX_LABEL

namespace CPPUI {
    namespace Wx {
        class Label: public Node {
            public:
            std::string text;
            Label(const char * text) {
                this->text = std::string(text);
            }
            Label(const std::string text) {
                this->text = text;
            }

            wxWindow * parentWindow(void) {
                ::CPPUI::Node * p = parent;
                while(p) {
                    Window * wx_window = dynamic_cast<Window *>(p);
                    if(wx_window) {
                        return (wxWindow *)(wx_window->ui);
                    }
                    p = p->parent;
                }
                return nullptr;
            }

            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    ((wxStaticText *)ui)->SetLabel(text);
                } else {
                    ui = new wxStaticText(parentWindow(), wxID_ANY, text);
                }
            }

            void destroy(bool direct) {
                if(direct && ui){
                    delete (wxStaticText *)ui;
                }
            }
       };
    }
    Wx::Label & Label(const char * text) {
        return *(new Wx::Label(text));
    }
    Wx::Label & Label(const std::string text) {
        return *(new Wx::Label(text));
    }
}

#endif
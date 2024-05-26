#include "base.hpp"

#include <wx/wx.h>

namespace CPPUI {
    namespace Wx {
        class Button: public Node {
            public:
            std::string text;
            Button(const std::string text) {
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

            void event_handler(wxCommandEvent & event) {
                onClicked();
            }

            void update(Node * prev) {
                if(prev && prev->ui) {
                    ui = prev->ui;
                    ((wxButton *)ui)->SetLabel(text);
                    ((wxButton *)ui)->Unbind(wxEVT_BUTTON, &Button::event_handler, (Button *)prev);
                } else {
                    ui = new wxButton(parentWindow(), wxID_ANY, text);
                }
                ((wxButton *)ui)->Bind(wxEVT_BUTTON, &Button::event_handler, this);
            }

            void destroy(bool direct) {
                if(direct && ui){
                    delete (wxButton *)ui;
                }
            }
       };
    }
    Wx::Button & Button(const std::string text) {
        return *(new Wx::Button(text));
    }
}

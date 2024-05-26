#include "base.hpp"

#include <iostream>
#include "unistd.h"

#include <wx/wx.h>

namespace CPPUI {
    class Application: public Wx::View {
        int argc;
        char **argv;

        public:
        Application(int argc, char *argv[]): Wx::View() {
            this->argc = argc;
            this->argv = argv;

            wxApp::SetInstance( new wxApp() );
            wxEntryStart( argc, argv );
        }

        void update(Node * prev) {
            View::update(prev);
        }

        void addChild(int idx, Node *child) {
            ((wxFrame *)child->outer())->Show();
        }

        void removeChild(int idx, Node *child) {
        }


        void start(void) {
            wxTheApp->OnRun();
            wxTheApp->OnExit();
            wxEntryCleanup();
        }
    };
}

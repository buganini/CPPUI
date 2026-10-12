#ifndef CPPUI_WX_APPLICATION
#define CPPUI_WX_APPLICATION
#include "base.hpp"
namespace CPPUI {
    namespace Wx { class RuntimeApp: public wxApp { public: bool OnInit() override { return true; } }; }
    class Application: public Wx::View {
        int argc;
        char ** argv;
        bool owned = false;
        wxApp * application = nullptr;
        wxEvtHandler quitContext;
        public:
        Application(int argc, char ** argv): argc(argc), argv(argv) {}
        ~Application() override {
            if(wxTheApp) wxTheApp->RemovePendingEventHandler(&quitContext);
            quitContext.DeletePendingEvents();
            clearChildren();
            // Finish our queued native destruction while wx is still initialized.
            // CallAfter handlers hold invalidatable native contexts, never nodes.
            if(application) { application->ProcessPendingEvents(); application->ProcessIdle(); }
            if(owned) { application->OnExit(); wxEntryCleanup(); }
        }
        void update(Node * previous) override {
            if(!application) {
                application = wxTheApp;
                if(!application) {
                    wxApp::SetInstance(new Wx::RuntimeApp());
                    if(!wxEntryStart(argc, argv)) throw std::runtime_error("wxWidgets initialization failed (a display is required)");
                    application = wxTheApp; owned = true;
                    if(!application->CallOnInit()) throw std::runtime_error("wxWidgets application initialization failed");
                }
            }
            Wx::View::update(previous);
        }
        void addChild(int, Node * child) override {
            if(!child->is_outoforder) if(auto window = static_cast<wxWindow *>(child->outer())) window->Show();
        }
        void removeChild(int, Node * child) override {
            if(!child->is_outoforder) if(auto window = static_cast<wxWindow *>(child->outer())) window->Hide();
        }
        void start() override { application->OnRun(); }
        void quit() {
            if(destroyed || retired_by) return;
            quitContext.CallAfter([this]() {
                if(destroyed || retired_by) return;
                closeRoots(children);
            });
        }
        private:
        static void closeRoots(const std::vector<Node *> * nodes) {
            for(auto node: *nodes) {
                if(node->is_virtual) closeRoots(node->children);
                else if(!node->is_outoforder) if(auto window = static_cast<wxWindow *>(node->outer())) window->Close();
            }
        }
    };
}
#endif

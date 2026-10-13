#include "cppui/wx/views.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/textfield.hpp"
#include "cppui/wx/application.hpp"
#include <wx/stopwatch.h>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    void settle() {
        wxStopWatch clock;
        do { wxTheApp->Yield(true); wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); wxMilliSleep(1); } while(clock.Time() < 10);
    }
    void edit(wxTextCtrl * field, const std::string & value) {
        field->SetValue(wxString::FromUTF8(value));
        wxCommandEvent event(CPPUI::Wx::commitEvent()); field->ProcessWindowEvent(event);
    }
    class Host: public CPPUI::Application {
        public:
        using Application::Application;
        void content() override {}
    };
    int editors = 0;
    class Editor: public CPPUI::Wx::View {
        public:
        std::string name;
        CPPUI::State<std::string> text{std::string("original")};
        int local = 0;
        explicit Editor(const std::string & name): name(name) { ++editors; }
        ~Editor() override { --editors; }
        void content() override {
            if(name == "empty") return;
            CPPUI::Label(name);
            CPPUI::TextField(text);
        }
        wxTextCtrl * field() { return static_cast<wxTextCtrl *>(children->at(1)->outer()); }
    };
    class Root: public CPPUI::Application {
        public:
        using Application::Application;
        std::vector<std::string> keys{"a", "b", "empty"};
        CPPUI::State<CPPUI::OptionalText> first{CPPUI::OptionalText("a")}, second{CPPUI::OptionalText("b")};
        bool show = true, external = false, other = false, missingID = false;
        int writes = 0;
        std::function<void()> action;
        void content() override {
            for(auto window: CPPUI::Window().size(480, 320)) if(show) {
                auto state = other ? &second : &first;
                CPPUI::Binding<CPPUI::OptionalText> binding([state]() { return static_cast<CPPUI::OptionalText>(*state); },
                    [this, state](const CPPUI::OptionalText & key) {
                        ++writes; auto invoke = action; *state = key; if(invoke) invoke();
                    });
                auto & container = external ? CPPUI::Views(binding) : CPPUI::Views();
                for(auto scope: container) for(const auto & key: keys) {
                    auto editor = new Editor(key);
                    if(!missingID) editor->id(key);
                }
            }
        }
        CPPUI::Wx::Views * views() { return static_cast<CPPUI::Wx::Views *>(children->front()->children->front()); }
        wxSimplebook * native() { return static_cast<wxSimplebook *>(views()->ui); }
        Editor * editor(int index) { return static_cast<Editor *>(views()->children->at(index)->children->front()->children->front()); }
    };
}
int main(int argc, char ** argv) {
    wxSetAssertHandler([](const wxString &, int, const wxString &, const wxString & condition, const wxString & message) {
        std::cerr << condition << ": " << message << '\n'; std::abort();
    });
    try {
        Host host(argc, argv); host.sync(); wxTheApp->SetExitOnFrameDelete(false);
        {
            Root root(argc, argv); root.sync();
            auto stack = root.native();
            require(stack->GetPageCount() == 4 && stack->GetSelection() == 0 && editors == 3, "internal model did not start empty or pages were not grouped");
            require(stack->GetPage(1)->GetChildren().size() == 2
                && stack->GetPage(3)->GetChildren().empty(), "multi-root or empty view was not retained as one page");
            auto editor = root.editor(0);
            wxWeakRef<wxWindow> removed = stack->GetPage(2);
            auto field = editor->field(); editor->local = 42;
            edit(field, "draft"); settle();
            root.views()->select("a"); settle();
            auto page = stack->GetCurrentPage();
            require(stack->GetSelection() == 1, "internal selection did not update");
            root.views()->select("b"); settle();
            require(!stack->GetPage(1)->IsShownOnScreen() && stack->GetPage(2)->IsShownOnScreen(),
                "selection did not hide/show native pages");
            editor->text = std::string("hidden draft"); settle();
            require(stack->GetSelection() == 2 && editor->field() == field && field->GetValue() == "hidden draft",
                "hidden view could not redraw independently");
            edit(field, "draft"); settle();
            root.views()->select("a"); settle();
            root.keys = {"empty", "a"}; root.sync(); settle();
            require(root.native() == stack && stack->GetCurrentPage() == page && stack->GetSelection() == 2
                && root.editor(1) == editor && editor->local == 42 && editor->field() == field && field->GetValue() == "draft"
                && !removed && editors == 2, "switch/reorder/removal lost retained view state or leaked pages");
            editor->redraw(); settle();
            require(field->GetValue() == "draft", "independent hidden-view redraw lost the editor buffer");
            root.views()->select(); settle();
            require(stack->GetSelection() == 0, "clearing internal selection did not show the empty page");
            root.views()->select("unknown"); settle();
            require(stack->GetSelection() == 0, "unknown selection did not show the empty page");
            root.keys.push_back("unknown"); root.sync();
            require(stack->GetSelection() == 3, "returning selected key was not restored");
            root.keys.clear(); root.sync(); settle();
            require(stack->GetPageCount() == 1 && editors == 0, "empty Views retained declared pages");
            root.keys = {"a"}; root.sync();
            for(const auto & invalid: std::vector<std::vector<std::string>>{{""}, {"a", "a"}}) {
                root.keys = invalid;
                bool rejected = false;
                try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
                require(rejected && root.native() == stack && stack->GetPageCount() == 2 && editors == 1
                    && CPPUI::getStack()->empty() && root.frames.empty(), "invalid page IDs corrupted mounted UI or construction scopes");
            }
            root.keys = {"a"}; root.missingID = true;
            bool rejected = false;
            try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && editors == 1 && stack->GetPageCount() == 2, "missing page ID was accepted or leaked declarations");
            root.missingID = false; root.sync();
            rejected = false;
            try { root.views()->select(""); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && stack->GetSelection() == 0, "invalid imperative selection damaged the container");
        }
        settle(); require(editors == 0, "internal Views teardown retained views");
        {
            Root root(argc, argv); root.external = true; root.sync();
            auto stack = root.native();
            require(stack->GetSelection() == 1 && root.writes == 0, "bound selection was not read silently");
            root.first = CPPUI::OptionalText("b"); settle();
            require(stack->GetSelection() == 2 && root.writes == 0, "state selection caused feedback");
            root.views()->select("a"); settle();
            require(*static_cast<CPPUI::OptionalText>(root.first) == "a" && root.writes == 1, "select did not write the bound model once");
            root.other = true; root.sync(); root.views()->select(); settle();
            require(!static_cast<CPPUI::OptionalText>(root.second) && *static_cast<CPPUI::OptionalText>(root.first) == "a"
                && stack->GetSelection() == 0 && root.writes == 2, "rebind wrote the old model or failed to clear selection");
            root.second = CPPUI::OptionalText("missing"); settle();
            root.keys.clear(); root.sync();
            require(stack->GetSelection() == 0 && *static_cast<CPPUI::OptionalText>(root.second) == "missing", "absent pages rewrote the selection");
            root.keys = {"missing"}; root.sync();
            require(stack->GetSelection() == 1, "bound missing key was not restored after insertion");
            root.second = CPPUI::OptionalText("");
            bool rejected = false;
            try { root.sync(); } catch(const std::invalid_argument &) { rejected = true; }
            require(rejected && stack->GetSelection() == 1 && editors == 1, "invalid bound selection damaged mounted content");
            root.second = CPPUI::OptionalText("missing"); settle();
            root.external = false; root.sync(); settle();
            require(stack->GetSelection() == 0, "returning to an internal model did not clear selection");
            root.views()->select("missing"); settle();
            require(stack->GetSelection() == 1, "replacement internal model could not select a page");
            root.external = true; root.sync();
            root.action = [&]() { root.show = false; root.sync(); };
            wxWeakRef<wxWindow> removed = stack;
            root.views()->select(); settle();
            require(!removed && editors == 0, "selection setter self-removal retained pages");
        }
        {
            std::unique_ptr<Root> root(new Root(argc, argv)); root->external = true; root->sync();
            root->action = [&root]() { root.reset(); };
            root->views()->select("b"); settle();
            require(!root && editors == 0, "selection setter could not destroy its owning view");
        }
        {
            Root root(argc, argv); root.sync();
            auto declaration = root.views();
            root.native()->Destroy(); settle();
            require(!declaration->ui, "external native deletion left stale Views access");
        }
        settle();
        require(editors == 0 && wxTopLevelWindows.empty(), "Views teardown leaked native roots or declarations");
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

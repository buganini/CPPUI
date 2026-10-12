#include "cppui/wx/application.hpp"
#include "cppui/wx/window.hpp"
#include "cppui/wx/layout.hpp"
#include "cppui/wx/label.hpp"
#include "cppui/wx/button.hpp"
#include "cppui/state.hpp"
#include <wx/weakref.h>
#include <sstream>
#include <stdexcept>

static void require(bool condition, const char * message) { if(!condition) throw std::runtime_error(message); }
static void pump() { for(int i = 0; i < 4; ++i) { wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); } }
static void click(wxWindow * button) { wxCommandEvent event(wxEVT_BUTTON, button->GetId()); event.SetEventObject(button); button->ProcessWindowEvent(event); }
class Child: public CPPUI::Wx::View {
    CPPUI::State<int> & value;
    public:
    int builds = 0;
    explicit Child(CPPUI::State<int> & value): value(value) {}
    void content() override { ++builds; CPPUI::Label(std::to_string(static_cast<int>(value))).id("nested"); }
};
class Root: public CPPUI::Application {
    public:
    using Application::Application;
    CPPUI::State<int> value{0};
    std::vector<std::string> order{"a", "b", "c"};
    int builds = 0, setups = 0, generation = 1, clicks = 0;
    bool styled = true, direct = false, nested = false, fail = false, removeOnClick = false, callbackFinished = false;
    void setup() override { ++setups; }
    void content() override {
        ++builds;
        for(auto window: CPPUI::Window().title("wx test").size(500, 300)) {
            if(direct) { CPPUI::Label("direct"); continue; }
            for(auto box: CPPUI::HBox()) {
                for(auto key: order) {
                    auto & button = CPPUI::Button(key);
                    button.id(key);
                    if(styled) button.layout(60, {}, 2, CPPUI::Insets(1, 2, 3, 4)).style(0, 0xffffff, 18);
                    int current = generation;
                    button.click([this, current]() {
                        clicks += current;
                        if(removeOnClick) { order.clear(); sync(); }
                        callbackFinished = true;
                    });
                }
                CPPUI::Label(std::to_string(static_cast<int>(value))).id("value");
                if(nested) (new Child(value))->id("child");
                CPPUI::Spacer();
            }
        }
        if(fail) throw std::runtime_error("expected builder failure");
    }
    CPPUI::Wx::Window * window() { return static_cast<CPPUI::Wx::Window *>(children->at(0)); }
    CPPUI::Wx::Layout * box() { return static_cast<CPPUI::Wx::Layout *>(window()->children->at(0)); }
    wxWindow * button(int i = 0) { return static_cast<wxWindow *>(box()->children->at(i)->outer()); }
};
int main(int argc, char ** argv) {
    wxSetAssertHandler([](const wxString &, int, const wxString &, const wxString & condition, const wxString & message) {
        std::cerr << "wx assertion: " << condition << ": " << message << '\n'; std::abort();
    });
    try {
        bool owned = argc > 1 && std::string(argv[1]) == "--owned";
        if(!owned) { wxApp::SetInstance(new wxApp()); require(wxEntryStart(argc, argv) && wxTheApp->CallOnInit(), "wx startup failed"); }
        auto host = wxTheApp;
        {
            Root root(argc, argv); root.sync();
            require(root.builds == 1 && root.setups == 1, "setup/build repeated");
            wxWeakRef<wxWindow> frame(root.window()->ui), a(root.button(0)), b(root.button(1)), c(root.button(2));
            auto originalFont = static_cast<CPPUI::Wx::Widget *>(root.box()->children->at(0))->native->font;
            require(a->GetParent() == root.window()->panel, "controls have incorrect parent");
            auto & mount = root.box()->mounts.at(0);
            require(mount.item->GetProportion() == 2 && mount.padding[0]->GetMinSize().y == 1 && mount.padding[3]->GetMinSize().x == 4, "padding/weight ignored");
            require(a->GetForegroundColour() == wxColour(0,0,0), "explicit black style ignored");
            root.order = {"c", "a"}; root.generation = 3; root.styled = false; root.sync(); pump();
            require(root.button(0) == c.get() && root.button(1) == a.get() && !b, "keyed reorder/removal lost native identity");
            require(a->GetFont() == originalFont && a->GetMinSize().x == -1, "omitted style/size did not reset");
            require(root.box()->mounts[0].item->GetProportion() == 0, "omitted weight did not reset");
            click(a); require(root.clicks == 3, "callback was duplicated or stale");
            const int builds = root.builds;
            for(int i = 1; i <= 20; ++i) root.value = i;
            require(root.builds == builds, "state reconciled synchronously");
            pump(); require(root.builds == builds + 1, "redraws were not coalesced");
            root.nested = true; root.sync();
            auto child = static_cast<Child *>(root.box()->children->at(3));
            require(child->builds == 1, "nested view failed to mount");
            root.value = 21; root.nested = false; root.sync(); pump();
            {
                std::ostringstream errors; auto old = std::cerr.rdbuf(errors.rdbuf());
                root.fail = true; root.redraw(); pump(); root.fail = false;
                std::cerr.rdbuf(old);
                require(errors.str().find("expected builder failure") != std::string::npos, "queued exception not reported");
                require(root.button() == c.get(), "failed build replaced old UI");
            }
            root.sync(); root.removeOnClick = true; root.sync(); click(c); pump();
            require(root.callbackFinished && !a && !c, "callback self-removal failed");
            for(int i = 0; i < 20; ++i) { root.order = {"x"}; root.sync(); root.order.clear(); root.sync(); pump(); }
            root.order = {"x"}; root.sync(); wxWeakRef<wxWindow> oldControl(root.button());
            root.direct = true; root.sync(); pump(); require(!oldControl, "detached sizer retained child windows");
            root.direct = false; root.sync();
            delete root.window()->ui;
            require(!frame && !root.window()->ui && !root.box()->sizer() && !root.button(), "external destruction left native observers");
            root.redraw(); // cancel pending render on teardown
        }
        require(owned ? wxTheApp == nullptr : wxTheApp == host, "application ownership violated");
        if(!owned) {
            pump(); require(wxTopLevelWindows.empty(), "native roots leaked");
            { Root root(argc, argv); root.sync(); root.quit(); pump(); require(!root.window()->ui, "queued quit did not close native root"); }
            wxTheApp->OnExit(); wxEntryCleanup();
        }
    } catch(const std::exception & error) { std::cerr << error.what() << '\n'; return 1; }
}

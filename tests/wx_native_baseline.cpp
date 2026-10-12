#include <wx/wx.h>
class App: public wxApp { public: bool OnInit() override { return true; } };
int main(int argc, char ** argv) {
 wxApp::SetInstance(new App()); if(!wxEntryStart(argc, argv) || !wxTheApp->CallOnInit()) return 1;
 auto frame = new wxFrame(nullptr, wxID_ANY, "baseline"); auto panel = new wxPanel(frame);
 auto box = new wxBoxSizer(wxVERTICAL); box->Add(new wxButton(panel, wxID_ANY, "button")); panel->SetSizer(box);
 frame->Show(); panel->Layout(); delete frame;
 wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); wxTheApp->OnExit(); wxEntryCleanup();
}

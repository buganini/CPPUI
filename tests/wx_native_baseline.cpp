#include <wx/wx.h>
#include <wx/html/htmlwin.h>
#include <wx/statline.h>
class App: public wxApp { public: bool OnInit() override { return true; } };
int main(int argc, char ** argv) {
 wxApp::SetInstance(new App()); if(!wxEntryStart(argc, argv) || !wxTheApp->CallOnInit()) return 1;
 auto frame = new wxFrame(nullptr, wxID_ANY, "baseline"); auto panel = new wxPanel(frame);
 auto box = new wxBoxSizer(wxVERTICAL); box->Add(new wxButton(panel, wxID_ANY, "button")); panel->SetSizer(box);
 auto combo=new wxComboBox(panel, wxID_ANY); combo->Append("One"); combo->SetSelection(0); box->Add(combo); box->Add(new wxCheckBox(panel, wxID_ANY,"check")); box->Add(new wxRadioButton(panel,wxID_ANY,"radio")); box->Add(new wxTextCtrl(panel, wxID_ANY, "text", wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_MULTILINE));
 auto readonly = new wxComboBox(panel,wxID_ANY,"",wxDefaultPosition,wxDefaultSize,0,nullptr,wxCB_READONLY); wxArrayString labels; labels.Add("A"); labels.Add("B"); readonly->Set(labels); readonly->SetSelection(0); box->Add(readonly);
 box->Add(new wxStaticLine(panel, wxID_ANY,wxDefaultPosition,wxSize(15,15),wxLI_HORIZONTAL)); box->Add(new wxStaticText(panel,wxID_ANY,"cell"));
 auto editor = new wxTextCtrl(panel,wxID_ANY,"initial",wxDefaultPosition,wxDefaultSize,wxTE_PROCESS_ENTER); box->Add(editor); editor->SetValue("typed"); combo->SetValue("free");
 auto gauge = new wxGauge(panel, wxID_ANY, 25); gauge->SetValue(12); box->Add(gauge);
 auto html = new wxHtmlWindow(panel); html->SetPage("<b>bold</b>"); box->Add(html);
 for(auto child: panel->GetChildren()) {
     child->SetFont(child->GetFont());
     child->SetForegroundColour(child->GetForegroundColour());
     child->SetBackgroundColour(child->GetBackgroundColour());
 }
 frame->Show(); panel->Layout(); wxTheApp->Yield(true);
 labels.Clear(); labels.Add("B"); labels.Add("A"); readonly->Set(labels); readonly->SetSelection(0);
 auto secondReadonly = new wxComboBox(panel,wxID_ANY,"",wxDefaultPosition,wxDefaultSize,0,nullptr,wxCB_READONLY);
 secondReadonly->Set(labels); box->Add(secondReadonly); panel->Layout(); wxTheApp->Yield(true);
 delete frame;
 wxTheApp->ProcessPendingEvents(); wxTheApp->ProcessIdle(); wxTheApp->OnExit(); wxEntryCleanup();
}

#ifndef CPPUI_WX_PAINTED_CONTROLS
#define CPPUI_WX_PAINTED_CONTROLS

#include <wx/wx.h>
#include <wx/generic/stattextg.h>
#include <wx/renderer.h>

namespace CPPUI { namespace Wx { namespace Painting {
    // Identifies the native host even when a text layer is nested in sizers.
    class StackPanel: public wxPanel {
    public:
        explicit StackPanel(wxWindow * parent): wxPanel(parent) {}
    };

    class StaticText: public wxGenericStaticText {
        bool stackLayer;
        void paint(wxPaintEvent &) { wxPaintDC dc(this); paintTo(dc); }
        static void drawText(wxDC & dc, const StaticText & window, wxPoint origin) {
            dc.SetFont(window.GetFont());
            dc.SetTextForeground(window.IsEnabled() ? window.GetForegroundColour()
                : wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
            for(const auto & line:wxSplit(window.GetLabelText(), '\n')) {
                dc.DrawText(line, origin);
                origin.y += dc.GetTextExtent(line.empty() ? wxString("W") : line).y;
            }
        }
        void drawLayers(wxDC & dc, wxSizer * sizer) const {
            if(!sizer) return;
            for(auto item:sizer->GetChildren()) {
                if(!item->IsShown()) continue;
                if(item->IsSizer()) drawLayers(dc,item->GetSizer());
                else if(item->IsWindow()) {
                    auto layer=dynamic_cast<StaticText *>(item->GetWindow());
                    if(!layer || !layer->stackLayer) continue;
                    const wxRect rectangle(layer->GetPosition()-GetPosition(),layer->GetSize());
                    wxDCClipper clip(dc,rectangle);
                    if(layer->customBackground) {
                        dc.SetPen(*wxTRANSPARENT_PEN);
                        dc.SetBrush(wxBrush(layer->GetBackgroundColour()));
                        dc.DrawRectangle(rectangle);
                    }
                    drawText(dc,*layer,rectangle.GetPosition());
                }
            }
        }
    public:
        bool customBackground=false;
        StaticText(wxWindow * parent, const wxString & label, bool stack=false): stackLayer(stack) {
            // MSW requires this before Create to make a transparent native window.
#ifdef __WXMSW__
            SetBackgroundStyle(stack ? wxBG_STYLE_TRANSPARENT : wxBG_STYLE_PAINT);
#else
            SetBackgroundStyle(wxBG_STYLE_PAINT);
#endif
            Create(parent,wxID_ANY,label,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE);
            Bind(wxEVT_PAINT,&StaticText::paint,this);
        }
        void paintTo(wxDC & dc) const {
            dc.SetBackground(wxBrush(stackLayer ? GetParent()->GetBackgroundColour() : GetBackgroundColour()));
            dc.Clear();
            if(stackLayer) drawLayers(dc,GetParent()->GetSizer());
            else drawText(dc,*this,wxPoint(0,0));
        }
    };

    // Cocoa's themed buttons ignore arbitrary colours. Keep button events and
    // keyboard/mouse activation while painting the explicitly styled control.
    class Button: public wxControl {
        bool mouseDown=false, keyDown=false, inside=false;
        void activate() {
            if(!IsEnabled()) return;
            wxCommandEvent event(wxEVT_BUTTON,GetId());event.SetEventObject(this);
            ProcessWindowEvent(event);
        }
        void cancel() {
            mouseDown=keyDown=false;
            if(HasCapture()) ReleaseMouse();
            Refresh();
        }
        void paint(wxPaintEvent &) { wxPaintDC dc(this); paintTo(dc); }
    protected:
        wxSize DoGetBestClientSize() const override {
            wxClientDC dc(const_cast<Button *>(this));dc.SetFont(GetFont());
            auto size=dc.GetMultiLineTextExtent(GetLabelText())+wxSize(20,12);
            size.IncTo(wxButton::GetDefaultSize());return size;
        }
    public:
        Button(wxWindow * parent, const wxString & label)
            :wxControl(parent,wxID_ANY,wxDefaultPosition,wxDefaultSize,wxBORDER_NONE) {
            SetBackgroundStyle(wxBG_STYLE_PAINT);
            const auto defaults=wxButton::GetClassDefaultAttributes();
            SetFont(defaults.font);SetOwnForegroundColour(defaults.colFg);SetBackgroundColour(defaults.colBg);
            SetLabel(label);
            Bind(wxEVT_PAINT,&Button::paint,this);
            Bind(wxEVT_LEFT_DOWN,[this](wxMouseEvent &) {
                if(!IsEnabled()) return;
                SetFocus();mouseDown=inside=true;CaptureMouse();Refresh();
            });
            Bind(wxEVT_LEFT_UP,[this](wxMouseEvent & event) {
                const bool click=mouseDown && GetClientRect().Contains(event.GetPosition());
                cancel();if(click) activate();
            });
            Bind(wxEVT_MOTION,[this](wxMouseEvent & event) {
                const bool over=GetClientRect().Contains(event.GetPosition());
                if(over!=inside) {inside=over;Refresh();}
                event.Skip();
            });
            Bind(wxEVT_LEAVE_WINDOW,[this](wxMouseEvent & event) {inside=false;Refresh();event.Skip();});
            Bind(wxEVT_MOUSE_CAPTURE_LOST,[this](wxMouseCaptureLostEvent &) {mouseDown=false;Refresh();});
            Bind(wxEVT_KEY_DOWN,[this](wxKeyEvent & event) {
                if(!IsEnabled()) return;
                if(event.GetKeyCode()==WXK_SPACE) {keyDown=true;Refresh();}
                else if(event.GetKeyCode()==WXK_RETURN || event.GetKeyCode()==WXK_NUMPAD_ENTER) activate();
                else event.Skip();
            });
            Bind(wxEVT_KEY_UP,[this](wxKeyEvent & event) {
                if(event.GetKeyCode()==WXK_SPACE) {const bool click=keyDown;keyDown=false;Refresh();if(click) activate();}
                else event.Skip();
            });
            Bind(wxEVT_SET_FOCUS,[this](wxFocusEvent & event) {Refresh();event.Skip();});
            Bind(wxEVT_KILL_FOCUS,[this](wxFocusEvent & event) {cancel();event.Skip();});
        }
        void SetLabel(const wxString & label) override {wxControl::SetLabel(label);InvalidateBestSize();Refresh();}
        bool SetFont(const wxFont & font) override {
            const bool changed=wxControl::SetFont(font);InvalidateBestSize();Refresh();return changed;
        }
        bool Enable(bool enable=true) override {if(!enable) cancel();return wxControl::Enable(enable);}
        void paintTo(wxDC & dc) const {
            auto background=GetBackgroundColour();
            if(!background.IsOk() || !background.Alpha()) background=wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
            if(keyDown || (mouseDown && inside)) background=background.ChangeLightness(90);
            dc.SetBackground(wxBrush(background));dc.Clear();
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW)));
            dc.DrawRectangle(GetClientRect());
            dc.SetFont(GetFont());
            dc.SetTextForeground(IsEnabled() ? GetForegroundColour() : wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
            dc.DrawLabel(GetLabelText(),GetClientRect(),wxALIGN_CENTER);
            if(HasFocus()) wxRendererNative::Get().DrawFocusRect(const_cast<Button *>(this),dc,GetClientRect().Deflate(4));
        }
    };
}}}
#endif

#ifndef CPPUI_WX_BASE
#define CPPUI_WX_BASE

#include "../view.hpp"
#include "drop.hpp"
#include <wx/wx.h>
#include <wx/weakref.h>
#include <memory>
#include <functional>

namespace CPPUI { namespace Wx {
    inline void reportException(const char * operation) {
        try { throw; }
        catch(const std::exception & error) { std::cerr << "CPPUI " << operation << ": " << error.what() << '\n'; }
        catch(...) { std::cerr << "CPPUI " << operation << ": unknown exception\n"; }
    }

    inline wxEventTypeTag<wxCommandEvent> commitEvent() {
        static const wxEventTypeTag<wxCommandEvent> type(wxNewEventType()); return type;
    }

    class View: public ::CPPUI::View {
        friend class Views;
        wxEvtHandler queue;
        bool dirty = false, updating = false, scheduled = false, setupDone = false;
        public:
        ~View() override { clearChildren(); }
        virtual void setup() {}
        void update(::CPPUI::Node * previous) override {
            if(destroyed || retired_by) return;
            const bool ownUpdate = !updating;
            if(ownUpdate) updating = true;
            try {
                if(!setupDone) { setup(); setupDone = true; }
                ::CPPUI::View::update(previous);
            } catch(...) { if(ownUpdate) { updating = false; dirty = false; } throw; }
            if(ownUpdate) { updating = false; if(dirty) redraw(); }
        }
        void redraw() override {
            if(destroyed || retired_by) return;
            dirty = true;
            if(updating || scheduled) return;
            scheduled = true;
            queue.CallAfter([this]() {
                scheduled = false;
                if(!dirty) return;
                try { sync(); } catch(...) { reportException("redraw"); }
            });
        }
        void sync() {
            if(destroyed || retired_by) return;
            if(updating) { dirty = true; return; }
            dirty = false; updating = true;
            try { ::CPPUI::View::redraw(); }
            catch(...) { updating = false; dirty = false; throw; }
            updating = false;
            if(dirty) redraw();
        }
        void run() override { sync(); start(); }
        void clearChildren() {
            destroyed = true; dirty = scheduled = false;
            if(wxTheApp) wxTheApp->RemovePendingEventHandler(&queue);
            queue.DeletePendingEvents();
            for(auto child: *children) recur_delete(child, true);
            children->clear();
        }
        void destroy(bool) override { if(wxTheApp) wxTheApp->RemovePendingEventHandler(&queue);
            queue.DeletePendingEvents(); }
        void queueCallback(std::function<void()> callback) {
            if(destroyed || !callback) return;
            queue.CallAfter([callback]() { try { callback(); } catch(...) { reportException("callback"); } });
        }
    };

    struct TimerQueue: std::enable_shared_from_this<TimerQueue> {
        struct Timer: wxTimer, std::enable_shared_from_this<Timer> {
            std::weak_ptr<TimerQueue> queue;
            Callback callback;
            void Notify() override {
                auto keep=shared_from_this();
                auto invoke=std::move(callback);
                if(auto owner=queue.lock()) {
                    auto & timers=owner->timers;
                    timers.erase(std::remove(timers.begin(),timers.end(),keep),timers.end());
                }
                try { invoke(); } catch(...) { reportException("posted callback"); }
            }
        };
        std::vector<std::shared_ptr<Timer>> timers;
        void schedule(int delay,Callback callback) {
            auto timer=std::make_shared<Timer>();
            timer->queue=shared_from_this(); timer->callback=std::move(callback);
            timers.push_back(timer); timer->StartOnce(delay);
        }
    };

    class Node;
    // One dispatch context per native object. Native pointers remain observers;
    // a callback may keep this context alive while removing its declaration.
    struct Native: wxEvtHandler, std::enable_shared_from_this<Native> {
        Node * node = nullptr;
        std::shared_ptr<TimerQueue> timers{new TimerQueue()};
        wxWindow * window = nullptr;
        wxWindow * owner = nullptr;
        wxSizer * sizer = nullptr;
        wxFont font;
        wxColour foreground, background;
        bool backgroundCustomized = false;
        std::function<void()> command, input, finish, resize;
        void sized(wxSizeEvent & event) {
            auto keep=shared_from_this();auto callback=resize;event.Skip();
            if(callback)try{callback();}catch(...){reportException("resize");}
        }
        void typed(wxCommandEvent &) {
            auto keep = shared_from_this(); auto callback = input;
            if(callback) try { callback(); } catch(...) { reportException("input"); }
        }
        void commit(wxCommandEvent &) {
            auto keep = shared_from_this(); auto callback = finish;
            if(callback) try { callback(); } catch(...) { reportException("commit"); }
        }
        void blur(wxFocusEvent & event) {
            auto keep = shared_from_this(); auto callback = finish; event.Skip();
            if(callback) try { callback(); } catch(...) { reportException("commit"); }
        }
        void destroyed(wxWindowDestroyEvent & event);
        void clicked(wxCommandEvent &) {
            auto keep = shared_from_this();
            auto callback = command;
            if(callback) try { callback(); } catch(...) { reportException("callback"); }
        }
        void mouse(wxMouseEvent & event) {
            auto keep = shared_from_this();
            auto callback = command;
            event.Skip();
            if(callback) try { callback(); } catch(...) { reportException("callback"); }
        }
    };

    class Node: public ::CPPUI::Node {
        public:
        std::shared_ptr<Native> native;
        int expandX = 0, expandY = 0;
        bool horizontal = false, vertical = false;
        virtual wxWindow * window() const { return nullptr; }
        virtual wxSizer * sizer() const { return nullptr; }
        virtual void invalidateNative() {}
        virtual void commit() {}
        virtual void rebindChild(Node *, Node *) {}
        virtual void applyVisibility(bool visible) {
            if(auto control=window()) if(supportsVisibility()) control->Show(visible && layoutVisible());
        }
        void update(::CPPUI::Node * previous) override {
            ::CPPUI::Node::update(previous);
            // An independently redrawing virtual view can retire declarations
            // before its physical parent's next sync. Rebind cached mounts now.
            if(previous && !is_outoforder) if(auto owner = dynamic_cast<Node *>(dom_parent))
                owner->rebindChild(static_cast<Node *>(previous), this);
        }
        ~Node() override { if(native && native->node == this) native->node = nullptr; }
        Node * physicalParent() const {
            auto p = parent;
            while(p && p != this) {
                if(auto wx = dynamic_cast<Node *>(p)) return wx;
                if(p == p->parent) break;
                p = p->parent;
            }
            return nullptr;
        }
        wxWindow * parentWindow() const {
            auto p = parent;
            while(p && p != this) {
                if(auto wx = dynamic_cast<Node *>(p)) if(auto w = static_cast<wxWindow *>(wx->inner())) return w;
                if(p == p->parent) break;
                p = p->parent;
            }
            throw std::logic_error("wx control requires a mounted native parent");
        }
        int expansion(bool x) const {
            auto p = physicalParent();
            if(p && (x ? p->horizontal : p->vertical) && layout_weight) return *layout_weight > 0 ? 4 : 0;
            return x ? expandX : expandY;
        }
        wxSize minSize() const { return wxSize(layout_width.valueOr(-1), layout_height.valueOr(-1)); }
        void releaseRetiredRefs() override {
            native.reset();
            ::CPPUI::Node::releaseRetiredRefs();
        }
        void transfer(Node * previous) {
            native = previous && previous->native ? previous->native : std::make_shared<Native>();
            native->node = this;
            Node::update(previous);
        }
        static void commitFocused() {
            wxWeakRef<wxWindow> focused(wxWindow::FindFocus());
            if(!focused) return;
            wxCommandEvent event(commitEvent()); focused->ProcessWindowEvent(event);
            if(focused && focused->GetParent()) focused->GetParent()->SetFocus();
        }
    };
    inline void Native::destroyed(wxWindowDestroyEvent & event) {
        if(event.GetEventObject() == owner) {
            timers->timers.clear();
            window = owner = nullptr; sizer = nullptr; command = {}; input = {}; finish = {}; resize = {};
            if(node) { node->clearRef(); node->invalidateNative(); }
        }
        event.Skip();
    }

    class Widget: public Node {
        public:
        wxWindow * ui = nullptr;
        ~Widget() override { clearRef(); }
        CommandPoster commandPoster() override {
            if(!ui || !wxTheApp) return {};
            auto app=wxTheApp;
            return [app](std::function<void()> callback) {
                app->CallAfter([callback]() { try { callback(); } catch(...) { reportException("ref"); } });
            };
        }
        TimerPoster timerPoster() override {
            if(!ui || !native) return {};
            std::weak_ptr<TimerQueue> weak=native->timers;
            return [weak](int delay,Callback callback) {
                if(auto queue=weak.lock()) queue->schedule(delay,std::move(callback));
            };
        }
        Widget() { is_terminal = true; layout_padding = Insets(3); }
        wxWindow * window() const override { return ui; }
        void * inner() override { return ui; }
        void * outer() override { return ui; }
        void invalidateNative() override { ui = nullptr; }
        void update(::CPPUI::Node * old) override {
            auto prev = static_cast<Widget *>(old);
            const bool reuse = prev && prev->ui;
            updateWidget(reuse ? prev : nullptr);
            transfer(reuse ? prev : nullptr);
            if(!reuse) {
                native->window = native->owner = ui;
                native->font = ui->GetFont(); native->foreground = ui->GetForegroundColour(); native->background = ui->GetBackgroundColour();
                ui->Bind(wxEVT_DESTROY, &Native::destroyed, native.get());
                bindEvents();
            }
            auto font = native->font;
            if(style_fontsize && *style_fontsize > 0) font.SetPointSize(*style_fontsize);
            if(style_fontfamily) font.SetFaceName(wxString::FromUTF8(*style_fontfamily));
            if(style_fontweight) font.SetWeight(*style_fontweight >= 600 ? wxFONTWEIGHT_BOLD : wxFONTWEIGHT_NORMAL);
            if(ui->GetFont() != font) ui->SetFont(font);
            auto foreground = style_color ? colour(*style_color) : native->foreground;
            if(ui->GetForegroundColour() != foreground) ui->SetOwnForegroundColour(foreground);
            if(style_bgcolor) {
                ui->SetBackgroundColour(colour(*style_bgcolor));
                native->backgroundCustomized = true;
            } else if(native->backgroundCustomized) {
                ui->SetBackgroundColour(wxNullColour);
                native->backgroundCustomized = false;
            }
            ui->SetMinSize(minSize());
            auto target=dynamic_cast<DropTarget *>(ui->GetDropTarget());
            if(dropHandlers.entered || dropHandlers.dropped) {
                if(!target) { target=new DropTarget(ui); ui->SetDropTarget(target); }
                target->handlers=dropHandlers;
            } else if(target) ui->SetDropTarget(nullptr);
            if(supportsVisibility() && (!is_visible || (prev && !prev->is_visible)))
                ui->Show(is_visible && ancestorsVisible());
            updateEvents();
        }
        virtual void updateWidget(Widget *) = 0;
        virtual void bindEvents() {}
        virtual void updateEvents() { native->command = onClicked; }
        void releaseRetiredRefs() override { ui = nullptr; Node::releaseRetiredRefs(); }
        void destroy(bool) override {
            if(ui) if(auto target=dynamic_cast<DropTarget *>(ui->GetDropTarget())) target->handlers={};
            dropHandlers={};
            clearRef();
            if(!native) return;
            native->timers->timers.clear();
            native->node = nullptr; native->command = {}; native->input = {}; native->finish = {}; native->resize = {};
            auto object = native;
            ui = nullptr;
            // Controls can be removed by their own command handler. Defer native
            // destruction until dispatch returns; external parent death cancels it.
            bool parentOwnsDeletion = false;
            for(auto ancestor = parent; ancestor && ancestor != this; ancestor = ancestor->parent) {
                auto widget = dynamic_cast<Widget *>(ancestor);
                if(widget && widget->destroyed && widget->ui) { parentOwnsDeletion = true; break; }
                if(ancestor == ancestor->parent) break;
            }
            if(!parentOwnsDeletion && object->window && wxTheApp) wxTheApp->CallAfter([object]() {
                if(object->window && !object->window->IsBeingDeleted()) object->window->Destroy();
            });
        }
        static wxColour colour(int rgb) { return wxColour((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255); }
    };

    struct Mount {
        Node * node = nullptr;
        wxSizerItem * item = nullptr, * content = nullptr;
        wxBoxSizer * row = nullptr;
        wxSizerItem * padding[4] = {};
    };
    inline void showMount(Mount & mount,bool visible=true) {
        visible=visible && mount.node->layoutVisible();
        if(mount.item) mount.item->Show(visible);
        // wxSizerItem::Show recursively shows descendants: reapply their own flags.
        mount.node->applyVisibility(visible);
    }
    inline Mount padded(Node * child, wxBoxSizer * outer) {
        Mount mount; mount.node = child;
        mount.padding[0] = outer->Add(0, 0);
        mount.row = new wxBoxSizer(wxHORIZONTAL);
        mount.padding[3] = mount.row->Add(0, 0);
        mount.content = child->sizer() ? mount.row->Add(child->sizer(), 1, wxEXPAND) : mount.row->Add(child->window(), 1, wxEXPAND);
        mount.padding[1] = mount.row->Add(0, 0);
        outer->Add(mount.row, 1, wxEXPAND);
        mount.padding[2] = outer->Add(0, 0);
        return mount;
    }
    inline void applyPadding(Mount & mount) {
        if(!mount.content) return;
        auto pad = mount.node->layout_padding.valueOr(Insets(0));
        mount.padding[0]->SetMinSize(0, pad.top); mount.padding[1]->SetMinSize(pad.right, 0);
        mount.padding[2]->SetMinSize(0, pad.bottom); mount.padding[3]->SetMinSize(pad.left, 0);
        mount.content->SetMinSize(mount.node->minSize());
    }
    inline void detachContent(Mount & mount) {
        if(!mount.row) return;
        if(mount.content->IsSizer()) mount.row->Detach(mount.content->GetSizer());
        else if(mount.content->IsWindow()) mount.row->Detach(mount.content->GetWindow());
    }

    class Spacer: public Node {
        public:
        Spacer() { is_terminal = true; layout_weight = 1; }
        void * inner() override { return nullptr; }
        void * outer() override { return nullptr; }
        void destroy(bool) override {}
    };

    class Layout: public Node {
        public:
        wxSizer * wxSizerObject = nullptr; // Native sizer; public layout() stays available.
        std::vector<Mount> mounts;
        Layout() { if(!dynamic_cast<Layout *>(physicalParent())) layout_padding = Insets(11); }
        wxSizer * sizer() const override { return wxSizerObject; }
        void * outer() override { return wxSizerObject; }
        void * inner() override { return nullptr; }
        void invalidateNative() override { wxSizerObject = nullptr; mounts.clear(); }
        virtual wxSizer * createSizer() = 0;
        void applyVisibility(bool visible) override {
            for(auto & mount:mounts) showMount(mount,visible && layoutVisible());
        }
        void rebindChild(Node * previous, Node * current) override {
            for(auto & mount: mounts) if(mount.node == previous) mount.node = current;
        }
        void update(::CPPUI::Node * old) override {
            auto prev = static_cast<Layout *>(old);
            const bool reuse = prev && prev->wxSizerObject;
            wxSizerObject = reuse ? prev->wxSizerObject : createSizer();
            if(reuse) mounts = std::move(prev->mounts);
            transfer(reuse ? prev : nullptr);
            if(!reuse) {
                native->sizer = wxSizerObject; native->owner = parentWindow();
                native->owner->Bind(wxEVT_DESTROY, &Native::destroyed, native.get());
            }
            wxSizerObject->SetMinSize(minSize());
        }
        virtual wxSizerItem * insert(int index, wxSizer * wrapper, Node *) { return wxSizerObject->Insert(index, wrapper); }
        void addChild(int index, ::CPPUI::Node * value) override {
            if(value->is_outoforder) return;
            auto child = dynamic_cast<Node *>(value);
            if(!child) throw std::logic_error("wx layout requires wx children");
            Mount mount; mount.node = child;
            if(dynamic_cast<Spacer *>(child)) mount.item = wxSizerObject->InsertStretchSpacer(index);
            else {
                auto wrapper = new wxBoxSizer(wxVERTICAL);
                mount = padded(child, wrapper); mount.item = insert(index, wrapper, child);
            }
            mounts.insert(mounts.begin() + index, mount);
        }
        void removeChild(int index, ::CPPUI::Node * child) override {
            if(child->is_outoforder || !wxSizerObject) return;
            auto mount = mounts.at(index); detachContent(mount);
            wxSizerObject->Remove(index);
            mounts.erase(mounts.begin() + index);
        }
        virtual void applyChildren() {
            int maxX = 0, maxY = 0;
            for(auto & mount: mounts) {
                applyPadding(mount);
                if(mount.node->layoutVisible()) {
                    maxX = std::max(maxX, mount.node->expansion(true)); maxY = std::max(maxY, mount.node->expansion(false));
                }
            }
            for(auto & mount: mounts) {
                showMount(mount,ancestorsVisible() && layoutVisible());
                auto child = mount.node;
                const int main = child->expansion(horizontal), cross = child->expansion(!horizontal);
                const int maximum = horizontal ? maxX : maxY;
                mount.item->SetFlag(cross ? int(wxEXPAND) : int(horizontal ? wxALIGN_CENTER_VERTICAL : wxALIGN_LEFT));
                mount.item->SetProportion(child->layoutVisible() ? child->layout_weight.valueOr(main == maximum && maximum > 0 ? 1 : 0) : 0);
            }
        }
        void postSync() override {
            if(!wxSizerObject) return;
            applyChildren(); wxSizerObject->Layout();
            if(native->owner) native->owner->Layout();
        }
        void postUpdate() override { postSync(); }
        void releaseRetiredRefs() override { wxSizerObject = nullptr; mounts.clear(); Node::releaseRetiredRefs(); }
        void destroy(bool direct) override {
            if(native) { native->node = nullptr; native->sizer = nullptr; }
            if(direct && wxSizerObject) { wxSizerObject->Clear(false); delete wxSizerObject; }
            wxSizerObject = nullptr; mounts.clear();
        }
    };

    class SingleChild: public Widget {
        public:
        wxWindow * panel = nullptr;
        std::shared_ptr<Mount> contentMount;
        SingleChild() { is_terminal = false; }
        void rebindChild(Node * previous, Node * current) override {
            if(contentMount && contentMount->node == previous) contentMount->node = current;
        }
        void * inner() override { return panel; }
        void invalidateNative() override { Widget::invalidateNative(); panel = nullptr; contentMount.reset(); }
        void update(::CPPUI::Node * old) override {
            auto prev = static_cast<SingleChild *>(old);
            contentMount = prev ? prev->contentMount : nullptr;
            Widget::update(old);
        }
        void addChild(int, ::CPPUI::Node * value) override {
            if(value->is_outoforder) return;
            if(contentMount) throw std::logic_error("wx container requires one physical content child; group controls in a layout");
            auto child = dynamic_cast<Node *>(value);
            auto wrapper = new wxBoxSizer(wxVERTICAL);
            contentMount = std::make_shared<Mount>(padded(child, wrapper));
            panel->SetSizer(wrapper);
        }
        void removeChild(int, ::CPPUI::Node * child) override {
            if(child->is_outoforder || !panel || !contentMount) return;
            detachContent(*contentMount); panel->SetSizer(nullptr); contentMount.reset();
        }
        void showContent(bool visible) {
            if(!panel || !contentMount) return;
            visible=visible && contentMount->node->layoutVisible();
            for(auto item:panel->GetSizer()->GetChildren()) item->Show(visible);
            showMount(*contentMount,visible);
        }
        void applyVisibility(bool visible) override {
            Widget::applyVisibility(visible);
            showContent(visible && layoutVisible());
        }
        void postSync() override {
            if(panel) {
                if(contentMount) applyPadding(*contentMount);
                showContent(ancestorsVisible() && layoutVisible());
                panel->Layout(); ui->Layout();
            }
        }
        void releaseRetiredRefs() override { panel = nullptr; contentMount.reset(); Widget::releaseRetiredRefs(); }
    };
}}
#endif

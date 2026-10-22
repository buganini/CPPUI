#ifndef CPPUI_WX_VIEWS
#define CPPUI_WX_VIEWS

#include "layout.hpp"
#include "../binding.hpp"
#include <wx/simplebook.h>
#include <set>

namespace CPPUI { namespace Wx {
    namespace detail {
        class ViewsPage: public SingleChild {
            public:
            ViewsPage() { layout_padding = Insets(0); }
            void updateWidget(Widget * prev) override {
                ui = prev ? prev->ui : new wxPanel(parentWindow());
                panel = ui;
            }
        };
    }
    class Views: public Widget {
        Optional<Binding<OptionalText>> model;
        std::shared_ptr<OptionalText> internal{new OptionalText()};
        OptionalText selected;
        bool prepared = false;
        static void validate(const OptionalText & key) {
            if(key && key->empty()) throw std::invalid_argument("Views selection must be a non-empty string or empty OptionalText");
        }
        public:
        Views() {
            if(!dynamic_cast<View *>(cppui_root)) throw std::logic_error("Views requires a wx Application or View");
            is_terminal = false;
            expandX = expandY = 3;
        }
        explicit Views(Binding<OptionalText> model): Views() {
            this->model = model;
            selected = model.get();
            validate(selected);
        }
        void prepare() override {
            if(!prepared) {
                std::set<std::string> keys;
                for(auto child: *children) {
                    if(!child->has_id || child->_id.empty() || !keys.insert(child->_id).second)
                        throw std::invalid_argument("Each Views child needs a unique, non-empty string ID");
                    if(child->is_outoforder) throw std::invalid_argument("Views pages must be ordinary widgets, layouts or views");
                }
                const auto count = children->size();
                std::vector<::CPPUI::Node *> pages, contents;
                pages.reserve(count); contents.reserve(count);
                {
                    auto rootScope = cppui_root->begin();
                    auto scope = begin();
                    // Complete allocations while the original children and new
                    // empty pages remain owned by this declaration for rollback.
                    for(std::size_t i = 0; i < count; ++i) {
                        auto page = new detail::ViewsPage();
                        page->id(children->at(i)->_id);
                        pages.push_back(page);
                        auto pageScope = page->begin();
                        auto content = new VBox();
                        content->layout({}, {}, {}, Insets(0));
                        content->children->reserve(1);
                        contents.push_back(content);
                    }
                }
                for(std::size_t i = 0; i < count; ++i) {
                    auto child = children->at(i);
                    child->parent = contents[i];
                    contents[i]->children->push_back(child);
                }
                children->swap(pages);
                prepared = true;
            }
            ::CPPUI::Node::prepare();
        }
        void updateWidget(Widget * previous) override {
            auto prev = static_cast<Views *>(previous);
            if(!model) {
                if(prev && !prev->model) internal = prev->internal;
                selected = *internal;
            }
            if(prev) ui = prev->ui;
            else {
                auto book = new wxSimplebook(parentWindow());
                book->AddPage(new wxPanel(book), ""); // Index zero is empty selection.
                ui = book;
            }
        }
        void addChild(int index, ::CPPUI::Node * child) override {
            static_cast<wxSimplebook *>(ui)->InsertPage(index + 1, static_cast<wxWindow *>(child->outer()), "");
        }
        void removeChild(int index, ::CPPUI::Node *) override {
            if(ui) static_cast<wxSimplebook *>(ui)->RemovePage(index + 1);
        }
        void postSync() override {
            int index = 0;
            if(selected) for(std::size_t i = 0; i < children->size(); ++i) {
                if(children->at(i)->_id == *selected) { index = static_cast<int>(i) + 1; break; }
            }
            if(ui) {
                static_cast<wxSimplebook *>(ui)->ChangeSelection(index);
                ui->Layout();
            }
        }
        bool refSelection() const override { return true; }
        void refCommand(RefCommand, const OptionalText & key, bool) override { select(key); }
        void select(OptionalText key = {}) {
            validate(key);
            if(destroyed || retired_by || !ui) return;
            auto binding = model;
            auto owned = internal;
            auto root = static_cast<View *>(cppui_root);
            wxWeakRef<wxWindow> alive(ui);
            wxWeakRef<wxEvtHandler> viewAlive(&root->queue);
            // A setter can synchronously remove this node or destroy its view.
            if(binding) binding->set(key);
            else *owned = key;
            if(alive && viewAlive) root->redraw();
        }
        void select(const std::string & key) { select(OptionalText(key)); }
        void select(const char * key) { select(key ? OptionalText(std::string(key)) : OptionalText()); }
    };
}}
namespace CPPUI {
    inline Wx::Views & Views() { return *new Wx::Views(); }
    inline Wx::Views & Views(Binding<OptionalText> model) { return *new Wx::Views(model); }
}
#endif

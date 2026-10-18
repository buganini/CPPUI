#include <functional>
#include <iosfwd>
#include <string>
#include <typeinfo>
#include <thread>
#include <vector>
#include <iterator>

#ifndef CPPUI_NODE_HEADER
#define CPPUI_NODE_HEADER

#include "context.hpp"
#include "modifiers.hpp"
#include "refs.hpp"
#include "drop.hpp"

namespace CPPUI {
    class Node;
    class View;

    // External inline linkage shares this thread-local stack across source files.
    inline std::vector<View *> * getStack() {
        thread_local std::vector<View *> stack;
        return &stack;
    }

    class Node: public Context {
        public:

        View * cppui_root = nullptr;
        Node * parent = nullptr;

        int classtype = 0;

        bool is_terminal = false;
        bool is_outoforder = false;
        bool is_reversed_order = false;
        bool is_view = false;
        bool is_virtual = false;
        bool is_movable = true;
        bool is_grid_layout = false;

        int grid_row = -1;
        int grid_column = -1;
        int grid_rowspan = -1;
        int grid_columnspan = -1;

        Node * dom_parent = nullptr;
        bool destroyed = false;
        Node * retired_by = nullptr;
        int debug = 0;
        std::string _id;
        bool has_id = false;
        std::string refName;
        std::shared_ptr<detail::RefTarget> refTarget;
        Node & ref(const std::string & name) {
            if(name.empty()) throw std::invalid_argument("ref name must be nonempty");
            refName=name; return *this;
        }
        void publishRef();
        void clearRef();
        virtual CommandPoster commandPoster() { return {}; }
        virtual TimerPoster timerPoster() { return {}; }
        const std::thread::id constructionThread=std::this_thread::get_id();
        Poster getPoster();
        virtual bool refSelection() const { return false; }
        virtual bool refEmptySelection() const { return false; }
        virtual bool refWindow() const { return false; }
        virtual void refCommand(RefCommand, const OptionalText &, bool) {}

        void * ui = nullptr;

        std::vector<Node *> * children = nullptr;
        std::function<void()> onClicked = [](){};
        detail::DropHandlers dropHandlers;
        Node & dragEnter(std::function<void(DropEvent &)> callback) { dropHandlers.entered=std::move(callback); return *this; }
        Node & drop(std::function<void(DropEvent &)> callback) {
            dropHandlers.dropped=std::move(callback); dropHandlers.extensions.reset(); return *this;
        }
        Node & drop(std::function<void(DropEvent &)> callback,const std::vector<std::string> & extensions) {
            auto normalized=detail::dropExtensions(extensions);
            dropHandlers.dropped=std::move(callback); dropHandlers.extensions=std::move(normalized); return *this;
        }

        Node();
        virtual ~Node();

        Optional<int> layout_width, layout_height, layout_weight;
        Optional<Insets> layout_padding, layout_margin;
        Optional<int> style_color, style_bgcolor, style_fontsize, style_fontweight;
        OptionalText style_fontfamily;

        Node & layout(Optional<int> width = {}, Optional<int> height = {}, Optional<int> weight = {},
                      Optional<Insets> padding = {}, Optional<Insets> margin = {}) {
            if((width && *width < 0) || (height && *height < 0) || (weight && *weight < 0))
                throw std::invalid_argument("Layout dimensions and weight must be non-negative");
            if(width) layout_width = width;
            if(height) layout_height = height;
            if(weight) layout_weight = weight;
            if(padding) layout_padding = padding;
            if(margin) layout_margin = margin;
            return *this;
        }
        Node & style(Optional<int> color = {}, Optional<int> bgColor = {}, Optional<int> fontSize = {},
                     Optional<int> fontWeight = {}, OptionalText fontFamily = {}) {
            if(fontSize && *fontSize < 0) throw std::invalid_argument("Font size must be non-negative");
            if(color) style_color = color;
            if(bgColor) style_bgcolor = bgColor;
            if(fontSize) style_fontsize = fontSize;
            if(fontWeight) style_fontweight = fontWeight;
            if(fontFamily) style_fontfamily = fontFamily;
            return *this;
        }
        // Non-Qt nodes accept backend options without applying them, like PUI.
        virtual Node & qt(const QtOptions &) { return *this; }

        Node & id(const char * name) {
            has_id = name != nullptr;
            _id = name ? name : "";
            return *this;
        }
        Node & id(const std::string & name) {
            has_id = true;
            _id = name;
            return *this;
        }
        virtual std::size_t typecode() const;
        bool operator==(Node &obj);
        virtual void enter(void);
        virtual void exit(void);

        virtual void update(Node * prev);
        virtual void prepare(void);
        virtual void releaseRetiredRefs(void);
        virtual void postUpdate(void);
        virtual void preSync(void);
        virtual void postSync(void);
        virtual void destroy(bool direct) = 0;

        virtual void * inner(void);
        virtual void * outer(void);

        virtual void addChild(int idx, Node *child);
        virtual void removeChild(int idx, Node *child);

        Node * getNode(void);
        bool findDomOffsetForNode(Node * node, int * offset);

        virtual void click(std::function<void()> callback);

        virtual void writeStream(std::ostream & out, int indent) const;
    };
}

#endif

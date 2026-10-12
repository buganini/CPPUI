#include "cppui/view.hpp"
#include <functional>
#include <set>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    int declarations = 0, natives = 0, nextSerial = 0;
    struct Native {
        int serial = ++nextSerial;
        std::string name;
        std::vector<Native *> ordered;
        std::set<Native *> outOfOrder;
        std::vector<int> removed;
        Native() { ++natives; }
        ~Native() { --natives; }
    };
    void mount(Native & parent, int index, CPPUI::Node * child) {
        auto native = static_cast<Native *>(child->outer());
        if(child->is_outoforder) parent.outOfOrder.insert(native);
        else {
            require(index >= 0 && index <= static_cast<int>(parent.ordered.size()), "mount offset exceeds ordered children");
            parent.ordered.insert(parent.ordered.begin() + index, native);
        }
    }
    void unmount(Native & parent, int index, CPPUI::Node * child) {
        auto native = static_cast<Native *>(child->outer());
        if(child->is_outoforder) parent.outOfOrder.erase(native);
        else {
            require(index >= 0 && index < static_cast<int>(parent.ordered.size()) && parent.ordered[index] == native,
                    "declaration order differs from mounted native order");
            parent.ordered.erase(parent.ordered.begin() + index);
        }
        parent.removed.push_back(native->serial);
    }
    class TestNode: public CPPUI::Node {
        public:
        TestNode() { ++declarations; }
        ~TestNode() { --declarations; }
        void update(CPPUI::Node * prev) override {
            if(!is_virtual) {
                ui = prev ? prev->ui : new Native();
                static_cast<Native *>(ui)->name = _id;
            }
            Node::update(prev);
        }
        void destroy(bool) override { delete static_cast<Native *>(ui); ui = nullptr; }
        void addChild(int index, CPPUI::Node * child) override { mount(*static_cast<Native *>(ui), index, child); }
        void removeChild(int index, CPPUI::Node * child) override { unmount(*static_cast<Native *>(ui), index, child); }
    };
    class Group: public TestNode { public: Group() { is_virtual = true; } };
    TestNode & node(const char * id) { auto result = new TestNode(); result->id(id); return *result; }
    class Root: public CPPUI::View {
        public:
        Native native;
        std::function<void()> build;
        ~Root() {
            for(auto child: *children) CPPUI::recur_delete(child, true);
            children->clear();
        }
        void content() override { build(); }
        void destroy(bool) override {}
        void addChild(int index, CPPUI::Node * child) override { mount(native, index, child); }
        void removeChild(int index, CPPUI::Node * child) override { unmount(native, index, child); }
    };
    void order(Root & root, std::initializer_list<const char *> expected) {
        require(root.native.ordered.size() == expected.size(), "wrong native child count");
        std::size_t i = 0;
        for(auto name: expected) require(root.native.ordered[i++]->name == name, "wrong native order");
    }
    void reversed() {
        Root root;
        root.is_reversed_order = true;
        root.build = []() { node("a"); node("b"); node("c"); };
        root.redraw();
        order(root, {"c", "b", "a"});
        const auto first = root.native.ordered;
        root.redraw();
        order(root, {"c", "b", "a"});
        require(root.native.ordered == first && root.native.removed.empty(), "unchanged reversed tree moved native objects");
        root.is_reversed_order = false;
        root.redraw();
        order(root, {"a", "b", "c"});
        require(root.native.ordered.back() == first.front(), "order change replaced native identity");
    }
    void nonMovable() {
        Root root;
        bool swap = false;
        root.build = [&]() {
            if(swap) node("b");
            node("fixed").is_movable = false;
            if(!swap) node("b");
        };
        root.redraw();
        const auto fixed = root.native.ordered.front()->serial;
        swap = true;
        root.redraw();
        order(root, {"b", "fixed"});
        require(root.native.ordered.back()->serial == fixed, "fixed native object was replaced unnecessarily");
        require(std::find(root.native.removed.begin(), root.native.removed.end(), fixed) == root.native.removed.end(),
                "non-movable native object was requeued");
    }
    void terminal() {
        Root root;
        root.build = []() {
            auto & leaf = node("terminal");
            leaf.is_terminal = true;
            for(auto scope: leaf) node("unmounted");
        };
        root.redraw();
        for(int i = 0; i < 5; ++i) root.redraw();
        require(declarations == 2 && natives == 2, "terminal redraw leaked unmounted declarations or mounted its children");
    }
    void grid() {
        Root root;
        root.is_grid_layout = true;
        bool shifted = false;
        root.build = [&]() {
            auto & second = node("second"); second.grid_row = 1; second.grid_column = 0;
            auto & first = node("first"); first.grid_row = 0; first.grid_column = shifted ? 1 : 0;
            node("unplaced");
        };
        root.redraw();
        order(root, {"first", "second"});
        const int serial = root.native.ordered.front()->serial;
        const auto second = root.native.ordered.back();
        require(declarations == 2, "unplaced grid declarations leaked");
        shifted = true;
        root.redraw();
        order(root, {"first", "second"});
        require(root.native.ordered.front()->serial != serial, "grid coordinates were omitted from identity");
        require(root.native.ordered.back() == second, "unchanged grid cell lost native identity");
    }
    void outOfOrderMove() {
        Root root;
        bool swap = false;
        auto group = []() {
            for(auto scope: (new Group())->id("group")) {
                for(auto inner: (new Group())->id("inner")) {
                    node("one"); node("menu").is_outoforder = true;
                }
                node("two");
            }
        };
        root.build = [&]() {
            if(!swap) group();
            node("tail");
            if(swap) group();
        };
        root.redraw();
        order(root, {"one", "two", "tail"});
        auto menu = *root.native.outOfOrder.begin();
        swap = true;
        root.redraw();
        order(root, {"tail", "one", "two"});
        require(root.native.outOfOrder.size() == 1 && *root.native.outOfOrder.begin() == menu,
                "out-of-order child was lost during a virtual move");
    }
}
int main() {
    try {
        reversed();
        nonMovable();
        terminal();
        grid();
        outOfOrderMove();
        require(declarations == 0 && natives == 0, "reconciliation teardown leaked resources");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

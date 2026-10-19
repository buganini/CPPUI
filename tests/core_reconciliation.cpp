#include "cppui/view.hpp"
#include "cppui/state.hpp"
#include <functional>
#include <set>
#include <map>
#include <random>
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
    class OffsetInner: public CPPUI::View {
        int & width;
    public:
        explicit OffsetInner(int & width):width(width) {}
        void content() override {for(int i=0;i<width;++i) node(std::to_string(i).c_str());}
        void destroy(bool) override {}
    };
    class OffsetFragment: public CPPUI::View {
        int & width;
    public:
        explicit OffsetFragment(int & width):width(width) {}
        void content() override {node("before");new OffsetInner(width);node("after");}
        void destroy(bool) override {}
    };
    void frozen() {
        Root root; bool show=false,remove=false; int width=2;
        root.build=[&]() {
            if(remove) return;
            for(auto scope:node("owner").visible(show)) (new OffsetFragment(width))->id("fragment");
            node("tail");
        };
        root.redraw();
        require(declarations==2 && natives==3,"initially hidden subtree allocated children");
        show=true;root.redraw();
        auto owner=root.children->front();auto fragment=owner->children->front();
        auto inner=static_cast<CPPUI::View *>(fragment->children->at(1));
        auto identity=inner->children->front()->ui;
        show=false;root.redraw();
        width=4;inner->redraw();
        require(inner->children->size()==2,"frozen child rebuilt independently");
        for(int i=0;i<4;++i) root.redraw();
        require(declarations==6,"discarded frozen proposals leaked");
        require(inner->dom_parent==root.children->front(),"retained child points at retired physical owner");
        show=true;root.redraw();
        require(inner->children->size()==4 && inner->children->front()->ui==identity,"thaw lost native identity or pending update");
        root.is_frozen=true;width=1;inner->redraw();
        require(inner->children->size()==4,"root freeze did not reach child");
        root.is_frozen=false;root.redraw();
        require(inner->children->size()==1,"root thaw did not refresh descendants");
        root.is_frozen=true;root.redraw();root.is_frozen=false;root.redraw();
        show=false;root.redraw();remove=true;root.redraw();
        require(declarations==0 && natives==1,"removing frozen tree leaked resources");
    }

    void offsetCache() {
        std::weak_ptr<const char> token;
        {
            Root root;std::vector<int> keys;std::map<int,int> widths;
            for(int i=0;i<20;++i) {keys.push_back(i);widths[i]=3;}
            bool reversed=false;
            root.build=[&]() {
                auto & owner=node("owner");owner.is_reversed_order=reversed;
                for(auto scope:owner) for(int key:keys) (new OffsetFragment(widths[key]))->id(std::to_string(key));
            };
            root.redraw();std::mt19937 random(42);
            auto verify=[&]() {
                auto owner=root.children->front();int expected=0;
                for(auto fragment:*owner->children) {
                    int offset=-1;
                    require(owner->findDomOffsetForNode(fragment,&offset) && offset==expected,"cached fragment offset differs from native order");
                    auto version=fragment->cachedDomOffsetVersion;
                    require(version && owner->findDomOffsetForNode(fragment,&offset) && fragment->cachedDomOffsetVersion==version,"offset cache failed to retain a hit");
                    auto inner=fragment->children->at(1);int direct=-1;
                    require(owner->findDomOffsetForNode(inner,&offset) && offset==expected+1,"nested cached offset differs from native order");
                    require(fragment->findDomOffsetForNode(fragment->children->back(),&direct)
                        && direct==1+int(inner->children->size()),"virtual ancestor cache has stale child count");
                    // A second owner's cache must never answer the first owner's query.
                    require(fragment->findDomOffsetForNode(inner,&direct) && direct==1
                        && owner->findDomOffsetForNode(inner,&offset) && offset==expected+1,"offset cache confused owners");
                    expected+=2+int(inner->children->size());
                }
                require(expected==int(static_cast<Native *>(owner->ui)->ordered.size()),"cached offsets differ from flattened native size");
            };
            for(int step=0;step<200;++step) {
                verify();auto owner=root.children->front();
                if(step%4==0) {
                    auto fragment=owner->children->at(random()%owner->children->size());
                    widths[std::stoi(fragment->_id)]=random()%6;
                    static_cast<CPPUI::View *>(fragment->children->at(1))->redraw();
                } else if(step%4==1) {
                    std::shuffle(keys.begin(),keys.end(),random);reversed=!reversed;root.redraw();
                } else if(step%4==2) {
                    int key=20+step;widths[key]=random()%6;keys.insert(keys.begin()+random()%(keys.size()+1),key);root.redraw();
                } else {
                    keys.erase(keys.begin()+random()%keys.size());root.redraw();
                }
                verify();
            }
            auto owner=root.children->front();auto fragment=owner->children->front();
            auto old=fragment->children->front();int offset;
            owner->findDomOffsetForNode(old,&offset);token=old->cachedDomOffsetVersion;
            static_cast<CPPUI::View *>(fragment)->redraw();verify();
            TestNode missing;require(!owner->findDomOffsetForNode(&missing,&offset),"failed lookup reused a successful cache");
            require(!owner->findDomOffsetForNode(nullptr,&offset)
                && offset==int(static_cast<Native *>(owner->ui)->ordered.size()),"null lookup lost its flattened size");
        }
        require(token.expired(),"offset token retained a removed tree");
    }

}
int main() {
    try {
        reversed();
        nonMovable();
        terminal();
        grid();
        outOfOrderMove();
        offsetCache();
        frozen();
        require(declarations == 0 && natives == 0, "reconciliation teardown leaked resources");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

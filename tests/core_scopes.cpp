#include "cppui/view.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    int live = 0;
    struct TestNode: CPPUI::Node {
        TestNode() { ++live; }
        ~TestNode() { --live; }
        void destroy(bool) override {}
    };
    struct FailedNode: TestNode {
        FailedNode() { throw std::runtime_error("constructor"); }
    };
    class Root: public CPPUI::View {
        public:
        int mode = 0;
        ~Root() {
            for(auto child: *children) CPPUI::recur_delete(child, true);
            children->clear();
        }
        void destroy(bool) override {}
        void nestedReturn() {
            for(auto scope: (new TestNode())->id("return")) {
                new TestNode();
                return;
            }
        }
        void content() override {
            for(auto scope: (new TestNode())->id("outer")) {
                for(auto inner: (new TestNode())->id("inner")) {
                    new TestNode();
                    if(mode == 1) throw std::runtime_error("builder");
                    if(mode == 2) new FailedNode();
                    break;
                }
                nestedReturn();
                require(frames.size() == 2, "break/return left a construction frame");
                continue;
            }
            new TestNode();
            require(frames.size() == 1, "normal exit left a construction frame");
        }
    };
}
int main() {
    try {
        {
            Root root;
            root.redraw();
            auto first = root.children->front();
            const int mounted = live;
            require(root.children->size() == 2, "scope escaped to the wrong parent");
            for(int mode: {1, 2}) {
                root.mode = mode;
                bool failed = false;
                try { root.redraw(); } catch(const std::runtime_error &) { failed = true; }
                require(failed, "builder did not report its failure");
                require(CPPUI::getStack()->empty() && root.frames.empty(), "failed builder left active stacks");
                require(live == mounted, "incomplete declarations leaked");
                require(root.children->front() == first, "failed builder discarded the previous tree");
            }
            root.mode = 0;
            root.redraw();
            require(live == mounted, "redraw after failure leaked declarations");
        }
        require(live == 0, "teardown leaked declarations");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

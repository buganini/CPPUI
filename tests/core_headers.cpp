#include "cppui/view.hpp"
#include "core_headers_support.hpp"

#include <stdexcept>
#include <thread>

namespace {
    void require(bool condition, const char * message) {
        if(!condition) {
            throw std::runtime_error(message);
        }
    }

    class Root: public CPPUI::View {
        CPPUI::State<int> & state;

        public:
        int builds = 0;
        int observed = 0;

        explicit Root(CPPUI::State<int> & value): state(value) {}

        ~Root() {
            for(CPPUI::Node * child: *children) {
                CPPUI::recur_delete(child, true);
            }
            children->clear();
        }

        void content() override {
            ++builds;
            observed = readStateFromOtherTranslationUnit(state);
            buildNodeFromOtherTranslationUnit();
        }

        void destroy(bool) override {}
    };
}

int main() {
    try {
        require(CPPUI::getStack() == stackFromOtherTranslationUnit(),
                "translation units must share the construction stack");

        CPPUI::State<int> state(1);
        Root root(state);
        root.redraw();
        require(root.children->size() == 1, "external builder lost its parent");
        require(root.children->front()->parent == &root, "wrong declaration parent");
        require(root.children->front()->dom_parent == &root, "wrong physical parent");
        require(root.observed == 1 && root.builds == 1, "initial state read failed");

        state = 2;
        require(root.observed == 2 && root.builds == 2,
                "external state read did not subscribe the active view");
        require(root.children->size() == 1, "redraw duplicated the child");

        root.enter();
        bool workerStackIsIndependent = false;
        std::thread worker([&]() {
            workerStackIsIndependent = CPPUI::getStack()->empty()
                && CPPUI::getStack() == stackFromOtherTranslationUnit();
        });
        worker.join();
        root.exit();
        require(workerStackIsIndependent, "construction stacks must remain thread-local");
        require(CPPUI::getStack()->empty(), "construction scope was not released");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

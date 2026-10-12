#include "cppui/state.hpp"

#include <memory>
#include <stdexcept>

namespace {
    void require(bool condition, const char * message) {
        if(!condition) {
            throw std::runtime_error(message);
        }
    }

    class PlainNode: public CPPUI::Node {
        public:
        void destroy(bool) override {}
    };

    class Subscriber: public CPPUI::View {
        CPPUI::State<int> & state;
        int & builds;

        public:
        int observed = 0;

        Subscriber(CPPUI::State<int> & value, int & count): state(value), builds(count) {}

        void content() override {
            ++builds;
            observed = state;
        }

        void destroy(bool) override {}
    };

    void viewDestroyedFirst() {
        CPPUI::State<int> state(1);
        int builds = 0;
        {
            Subscriber view(state, builds);
            require(view.cppui_root == &view && view.parent == &view,
                    "a fully constructed View must establish its own root");
            view.redraw();
            state = 2;
            require(view.observed == 2 && builds == 2, "subscriber did not redraw");
        }
        state = 3;
        require(builds == 2, "destroyed view received a notification");
    }

    void stateDestroyedFirst() {
        std::unique_ptr<CPPUI::State<int>> state(new CPPUI::State<int>(1));
        int builds = 0;
        Subscriber view(*state, builds);
        view.redraw();
        require(view.stateSubscriptions.size() == 1, "view did not track its state");
        state.reset();
        require(view.stateSubscriptions.empty(), "destroyed state remained subscribed");
        // The application must not redraw a view whose model it has destroyed.
        // Its destructor must nevertheless be safe and must not touch that model.
    }

    void ownedIdentity() {
        PlainNode first, second, anonymous;
        std::string name(100, 'a');
        first.id(name.c_str());
        second.id(std::string(100, 'a'));
        name.assign(100, 'b');
        require(first == second, "node identity retained the caller's text buffer");
        first.id("");
        require(!(first == anonymous), "an empty ID must remain distinct from no ID");
        first.id(nullptr);
        require(first == anonymous, "null ID did not reset explicit identity");
    }
}

int main() {
    try {
        PlainNode standalone;
        require(standalone.cppui_root == nullptr,
                "a standalone Node must not pretend to be a View");
        require(standalone.parent == &standalone, "standalone node parent changed");
        viewDestroyedFirst();
        stateDestroyedFirst();
        ownedIdentity();
        require(CPPUI::getStack()->empty(), "a test left an active construction scope");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

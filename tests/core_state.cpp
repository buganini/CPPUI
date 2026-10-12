#include "cppui/state.hpp"
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    class Conditional: public CPPUI::View {
        public:
        CPPUI::State<int> & first;
        CPPUI::State<int> & second;
        bool useFirst = true;
        bool fail = false;
        int builds = 0;
        int value = 0;
        Conditional(CPPUI::State<int> & a, CPPUI::State<int> & b): first(a), second(b) {}
        void destroy(bool) override {}
        void content() override {
            ++builds;
            if(fail) throw std::runtime_error("builder");
            value = useFirst ? static_cast<int>(first) : static_cast<int>(second);
        }
    };
    class Mutation: public CPPUI::View {
        public:
        CPPUI::State<int> & state;
        int rejected = 0;
        explicit Mutation(CPPUI::State<int> & value): state(value) {}
        void destroy(bool) override {}
        void content() override {
            try { state = 2; } catch(const CPPUI::StateMutationInViewBuilderError &) { ++rejected; }
            try { state += 3; } catch(const CPPUI::StateMutationInViewBuilderError &) { ++rejected; }
            try { state -= 4; } catch(const CPPUI::StateMutationInViewBuilderError &) { ++rejected; }
        }
    };
}
int main() {
    try {
        CPPUI::State<int> first{1}, second{2};
        Conditional view(first, second);
        view.redraw();
        view.redraw(); // An already tracked read must still count as a dependency.
        first = 3;
        require(view.value == 3 && view.builds == 3, "unchanged dependencies were lost");
        view.useFirst = false;
        view.redraw();
        require(view.stateSubscriptions.size() == 1, "stale conditional dependency remained");
        first = 4;
        require(view.builds == 4, "unused state triggered a redraw");
        second = 5;
        require(view.builds == 5 && view.value == 5, "new conditional dependency was not subscribed");
        view.fail = true;
        try { view.redraw(); } catch(const std::runtime_error &) {}
        view.fail = false;
        second = 6;
        require(view.value == 6, "builder failure lost the previous subscription");
        view.fail = true;
        try { second = 7; } catch(const std::runtime_error &) {}
        view.fail = false;
        second = 8;
        require(view.value == 8, "failed notification detached the subscriber permanently");

        Mutation mutation(first);
        mutation.redraw();
        require(mutation.rejected == 3, "builder mutation was not rejected before assignment");
        require(static_cast<int>(first) == 4, "rejected mutation changed the state");
        first += 1;
        first -= 1;
        require(static_cast<int>(first) == 4, "ordinary event-time mutation was rejected");
        require(CPPUI::getStack()->empty(), "mutation error leaked a builder scope");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

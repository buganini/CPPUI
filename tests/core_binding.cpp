#include "cppui/binding.hpp"
#include <memory>
#include <stdexcept>

namespace {
    void require(bool condition, const char * message) { if(!condition) throw std::runtime_error(message); }
    class Reader: public CPPUI::View {
        public:
        CPPUI::Binding<int> binding;
        int value = 0, builds = 0;
        explicit Reader(CPPUI::Binding<int> binding): binding(binding) {}
        void content() override { ++builds; value = binding.get(); }
        void destroy(bool) override {}
    };
}
int main() {
    try {
        CPPUI::State<int> first{1}, second{2};
        Reader reader(first);
        reader.redraw();
        CPPUI::Binding<int>(first).set(3);
        require(reader.value == 3 && reader.builds == 2, "State adapter lost reactive reads or writes");
        reader.binding = CPPUI::Binding<int>(second);
        reader.redraw();
        first = 4;
        require(reader.builds == 3, "rebinding retained the previous State dependency");
        second = 5;
        require(reader.value == 5 && reader.builds == 4, "new State dependency was not tracked");

        int external = 6, writes = 0;
        CPPUI::Binding<int> adapter([&]() { return external; }, [&](const int & value) {
            if(value < 0) throw std::invalid_argument("negative");
            ++writes;
            external = value;
        });
        adapter.set(7);
        require(adapter.get() == 7 && writes == 1, "external binding duplicated its write");
        try { adapter.set(-1); } catch(const std::invalid_argument &) {}
        require(adapter.get() == 7 && writes == 1, "rejected external value changed the model");

        std::unique_ptr<CPPUI::Binding<int>> self;
        auto payload = std::make_shared<int>(9);
        std::weak_ptr<int> observer = payload;
        self.reset(new CPPUI::Binding<int>([]() { return 0; }, [&, payload](const int &) {
            self.reset();
            require(*payload == 9, "active setter capture was reclaimed during its call");
        }));
        payload.reset();
        self->set(1);
        require(observer.expired(), "completed setter retained its captures");
        bool rejected = false;
        try { CPPUI::Binding<int> invalid({}, [](const int &) {}); }
        catch(const std::invalid_argument &) { rejected = true; }
        require(rejected, "empty getter was accepted");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

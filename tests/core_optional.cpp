#include "cppui/optional.hpp"
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    int alive = 0;
    struct Value {
        int number;
        explicit Value(int number): number(number) { ++alive; }
        Value(const Value & other): number(other.number) { ++alive; }
        ~Value() { --alive; }
    };
}
int main() {
    try {
        CPPUI::OptionalText absent;
        CPPUI::OptionalText empty{std::string()};
        require(!absent && empty && empty->empty(), "absent and present-empty values are not distinct");
        require(absent.valueOr("fallback") == "fallback" && empty.valueOr("fallback").empty(), "incorrect fallback");
        bool rejected = false;
        try { absent.value(); } catch(const std::logic_error &) { rejected = true; }
        require(rejected, "empty access must fail explicitly");
        {
            CPPUI::Optional<Value> a;
            require(alive == 0, "absent optional constructed a value");
            a = Value(7);
            CPPUI::Optional<Value> b(a);
            b->number = 9;
            require(a->number == 7 && b->number == 9 && alive == 2, "optional copies share owned data");
            CPPUI::Optional<Value> c(std::move(a));
            require(!a && c->number == 7 && alive == 2, "move did not transfer the value");
            b = b;
            require(b->number == 9 && alive == 2, "self-assignment changed the value or leaked");
            c.reset();
            require(!c && alive == 1, "reset did not reclaim its value");
        }
        require(alive == 0, "optional teardown leaked values");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

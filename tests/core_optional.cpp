#include "cppui/optional.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
    bool trackAllocations = false;
    std::size_t allocations = 0;
}
void * operator new(std::size_t size) {
    if(trackAllocations) ++allocations;
    if(auto memory = std::malloc(size ? size : 1)) return memory;
    throw std::bad_alloc();
}
void operator delete(void * memory) noexcept { std::free(memory); }
void * operator new[](std::size_t size) { return ::operator new(size); }
void operator delete[](void * memory) noexcept { ::operator delete(memory); }
#if __cplusplus >= 201402L
void operator delete(void * memory, std::size_t) noexcept { ::operator delete(memory); }
void operator delete[](void * memory, std::size_t) noexcept { ::operator delete[](memory); }
#endif

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    int alive = 0;
    struct Value {
        int number;
        explicit Value(int number): number(number) { ++alive; }
        Value(const Value & other): number(other.number) { ++alive; }
        ~Value() { --alive; }
    };
    struct alignas(64) Aligned {
        const int number;
        explicit Aligned(int number): number(number) {}
        Aligned * operator&() { return nullptr; }
        const Aligned * operator&() const { return nullptr; }
        static void * operator new(std::size_t) = delete;
        static void * operator new(std::size_t, void *) = delete;
    };
    struct CopyOnly {
        const int number;
        explicit CopyOnly(int number): number(number) {}
        CopyOnly(const CopyOnly &) noexcept = default;
        CopyOnly(CopyOnly &&) = delete;
        CopyOnly & operator=(const CopyOnly &) = delete;
    };

    int throwingAlive = 0, copiesBeforeThrow = -1;
    struct Throwing {
        const int number;
        explicit Throwing(int number): number(number) { ++throwingAlive; }
        Throwing(const Throwing & other): number(other.number) {
            if(copiesBeforeThrow == 0) throw std::runtime_error("copy");
            if(copiesBeforeThrow > 0) --copiesBeforeThrow;
            ++throwingAlive;
        }
        Throwing(Throwing &&): number(0) { throw std::runtime_error("move should use the copy fallback"); }
        ~Throwing() { --throwingAlive; }
    };
    int movingAlive = 0, movesBeforeThrow = -1;
    struct ThrowingMoveOnly {
        int number;
        explicit ThrowingMoveOnly(int number): number(number) { ++movingAlive; }
        ThrowingMoveOnly(const ThrowingMoveOnly &) = delete;
        ThrowingMoveOnly(ThrowingMoveOnly && other): number(other.number) {
            if(movesBeforeThrow == 0) throw std::runtime_error("move");
            if(movesBeforeThrow > 0) --movesBeforeThrow;
            other.number = -1;
            ++movingAlive;
        }
        ~ThrowingMoveOnly() { --movingAlive; }
    };
    static_assert(alignof(CPPUI::Optional<Aligned>) >= alignof(Aligned), "inline storage lost alignment");
    static_assert(std::is_nothrow_move_constructible<CPPUI::Optional<int>>::value, "scalar move should not throw");
    static_assert(std::is_nothrow_move_assignable<CPPUI::Optional<int>>::value, "scalar assignment should not throw");
    static_assert(std::is_nothrow_move_constructible<CPPUI::Optional<CopyOnly>>::value, "nothrow copy fallback was lost");
    static_assert(!std::is_nothrow_move_constructible<CPPUI::Optional<Throwing>>::value, "throwing copy fallback is noexcept");
    static_assert(!std::is_nothrow_move_constructible<CPPUI::Optional<ThrowingMoveOnly>>::value, "throwing move is noexcept");
    static_assert(!std::is_nothrow_move_assignable<CPPUI::Optional<ThrowingMoveOnly>>::value, "throwing assignment is noexcept");

    template<class Action>
    void rejects(Action action) {
        bool rejected = false;
        try { action(); } catch(const std::runtime_error &) { rejected = true; }
        require(rejected, "payload construction exception was swallowed");
    }
    template<class T>
    bool storedInline(const CPPUI::Optional<T> & value) {
        const auto begin = reinterpret_cast<std::uintptr_t>(std::addressof(value));
        const auto payload = reinterpret_cast<std::uintptr_t>(std::addressof(*value));
        return payload >= begin && payload + sizeof(T) <= begin + sizeof(value);
    }

    void inlineValues() {
        int sum = 0;
        allocations = 0;
        trackAllocations = true;
        {
            CPPUI::Optional<int> values[128];
            for(int i = 0; i < 128; ++i) {
                values[i] = i;
                auto copy = values[i];
                auto moved = std::move(copy);
                values[i] = std::move(moved);
                sum += values[i].valueOr(-1);
                values[i].reset();
            }
        }
        trackAllocations = false;
        require(sum == 127 * 128 / 2 && allocations == 0, "scalar Optional operations allocated heap storage");
        CPPUI::Optional<int> scalar(42);
        require(storedInline(scalar), "scalar payload is not inside its Optional");
        CPPUI::Optional<Aligned> aligned(Aligned(7));
        require(aligned->number == 7 && storedInline(aligned), "over-aligned or address-overloaded payload is not inline");
        require(reinterpret_cast<std::uintptr_t>(aligned.operator->()) % alignof(Aligned) == 0, "payload address is misaligned");
        aligned = Aligned(9);
        require(aligned->number == 9, "non-assignable payload could not be replaced");
        CPPUI::Optional<const int> constant(1);
        constant = 2;
        require(*constant == 2, "const payload could not be reconstructed");
    }

    void movesAndAliases() {
        CPPUI::Optional<std::unique_ptr<int>> source{std::unique_ptr<int>(new int(42))};
        CPPUI::Optional<std::unique_ptr<int>> moved(std::move(source));
        require(!source && **moved == 42, "move-only payload was lost");
        auto & self = moved;
        moved = std::move(self);
        require(moved && **moved == 42, "self-move lost the value");
        moved = std::move(*moved);
        require(moved && **moved == 42, "assignment from an aliased moved payload lost the value");
        source = std::move(moved);
        require(!moved && **source == 42, "move assignment did not empty the source");
        source = {};
        require(!source, "empty assignment did not destroy the payload");
        const CopyOnly value(23);
        CPPUI::Optional<CopyOnly> copy(value);
        CPPUI::Optional<CopyOnly> copyMoved(std::move(copy));
        require(!copy && copyMoved->number == 23, "copy-only payload could not be transferred");
        copyMoved = *copyMoved;
        require(copyMoved->number == 23, "assignment from an aliased const payload lost the value");
        CPPUI::OptionalText text(std::string(100, 'x'));
        CPPUI::OptionalText independent(text);
        independent->front() = 'y';
        require(text->front() == 'x', "string copies share contents");
        CPPUI::OptionalText textMoved(std::move(text));
        require(!text && textMoved->size() == 100, "string move lost its contents");
        CPPUI::Optional<CPPUI::Optional<int>> nested{CPPUI::Optional<int>()};
        require(nested && !*nested, "present-empty nested Optional was flattened");
        auto nestedMoved = std::move(nested);
        require(!nested && nestedMoved && !*nestedMoved, "nested move lost the presence state");
    }

    void failures() {
        {
            const Throwing original(7), replacement(9);
            CPPUI::Optional<Throwing> destination(original), source(replacement);
            copiesBeforeThrow = 0;
            rejects([&]() { CPPUI::Optional<Throwing> failed(source); });
            rejects([&]() { CPPUI::Optional<Throwing> failed(std::move(source)); });
            rejects([&]() { destination = source; });
            require(destination->number == 7 && source->number == 9 && throwingAlive == 4,
                    "failed incoming copy changed values or leaked");
            copiesBeforeThrow = 1;
            rejects([&]() { destination = source; });
            require(!destination && source->number == 9 && throwingAlive == 3,
                    "failed replacement was not left empty or destroyed an unconstructed payload");
            copiesBeforeThrow = -1;
            destination = std::move(source);
            require(!source && destination->number == 9 && throwingAlive == 3, "copy fallback did not empty the moved source");
            destination.reset();
            destination.reset();
        }
        require(throwingAlive == 0, "throwing copy payload leaked");
        {
            CPPUI::Optional<ThrowingMoveOnly> destination(ThrowingMoveOnly(7)), source(ThrowingMoveOnly(9));
            movesBeforeThrow = 0;
            rejects([&]() { CPPUI::Optional<ThrowingMoveOnly> failed(std::move(source)); });
            rejects([&]() { destination = std::move(source); });
            require(destination->number == 7 && source->number == 9 && movingAlive == 2,
                    "failed incoming move destroyed live payloads");
            movesBeforeThrow = 1;
            rejects([&]() { destination = std::move(source); });
            require(!destination && !source && movingAlive == 0, "failed replacement move leaked or left an invalid value");
            movesBeforeThrow = -1;
            destination = ThrowingMoveOnly(5);
            require(destination->number == 5, "Optional could not be reused after a failed move");
        }
        require(movingAlive == 0, "throwing move payload leaked");
    }
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
        inlineValues();
        movesAndAliases();
        failures();
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#ifndef CPPUI_BINDING
#define CPPUI_BINDING

#include "state.hpp"
#include <functional>
#include <utility>

namespace CPPUI {
    template<class T>
    class Binding {
        std::function<T()> getter;
        std::function<void(const T &)> setter;

        public:
        Binding(std::function<T()> get, std::function<void(const T &)> set)
            : getter(std::move(get)), setter(std::move(set)) {
            if(!getter || !setter) throw std::invalid_argument("Binding needs a getter and setter");
        }
        // Borrow State, preserving its dependency tracking and mutation rules.
        Binding(State<T> & state): Binding([&state]() { return static_cast<T>(state); },
                                          [&state](const T & value) { state = value; }) {}
        T get() const {
            auto invoke = getter;
            return invoke();
        }
        void set(const T & value) const {
            // A setter may redraw and release the Binding that initiated it.
            auto invoke = setter;
            invoke(value);
        }
    };
}

#endif

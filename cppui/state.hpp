#include <iostream>
#include <vector>

#ifndef CPPUI_STATE_HEADER
#define CPPUI_STATE_HEADER

#define DEBUG_CPPUI_STATE 0

#include "view_header.hpp"

namespace CPPUI {
    class View;

    template <typename T>
    class State: public Context {
        T value;
        std::vector<View *> listeners;

        inline void notifyListeners() {
#if DEBUG_CPPUI_STATE > 0
            std::cout << "noitify " << listeners.size() << std::endl;
#endif
            std::vector<View *> views(listeners);
            listeners.clear();
            for(View *view: views){
                std::cout << "redraw " << view << std::endl;
                view->redraw();
            }
        }

        public:
        State(T initValue) {
            value = initValue;
        }

        operator T() {
            std::vector<View *> * stack = getStack();
            if(stack->size()==0){
                return value;
            }
            View * view = stack->back();
            for(View *v: listeners){
                if(v==view){
                    return value;
                }
            }
            listeners.push_back(view);
            return value;
        }

        State<T> & operator=(const T & rhs) {
            value = rhs;
            notifyListeners();
            return *this;
        }

        State<T> & operator+=(const T& rhs) {
            value += rhs;
            notifyListeners();
            return *this;
        }

        State<T> & operator-=(const T& rhs) {
            value -= rhs;
            notifyListeners();
            return *this;
        }

        void enter(void) {

        }

        void exit(void) {

        }
    };
}

#endif
#include <iostream>
#include <algorithm>
#include <vector>
#include <stdexcept>

#ifndef CPPUI_STATE_HEADER
#define CPPUI_STATE_HEADER

#define DEBUG_CPPUI_STATE 0

#include "view.hpp"

namespace CPPUI {
    class View;

    class StateMutationInViewBuilderError: public std::logic_error {
        public:
        StateMutationInViewBuilderError()
            : std::logic_error("State cannot be mutated while building a view") {}
    };

    template <typename T>
    class State: public Context, public StateBase {
        T value;
        std::vector<View *> listeners;
        std::vector<View *> pendingNotifications;

        static void checkMutation() {
            if(!getStack()->empty()) throw StateMutationInViewBuilderError();
        }

        inline void notifyListeners() {
#if DEBUG_CPPUI_STATE > 0
            std::cout << "noitify " << listeners.size() << std::endl;
#endif
            pendingNotifications = listeners;
            while(!pendingNotifications.empty()) {
                View * view = pendingNotifications.front();
                pendingNotifications.erase(pendingNotifications.begin());
                std::cout << "redraw " << view << std::endl;
                view->redraw();
            }
        }

        public:
        State(T initValue) {
            value = initValue;
        }

        ~State() {
            std::vector<View *> views(listeners);
            views.insert(views.end(), pendingNotifications.begin(), pendingNotifications.end());
            listeners.clear();
            pendingNotifications.clear();
            for(View * view: views) {
                view->untrackState(this);
            }
        }

        void removeListener(View * view) {
            listeners.erase(
                std::remove(listeners.begin(), listeners.end(), view),
                listeners.end()
            );
            pendingNotifications.erase(
                std::remove(pendingNotifications.begin(), pendingNotifications.end(), view),
                pendingNotifications.end()
            );
        }

        operator T() {
            std::vector<View *> * stack = getStack();
            if(stack->size()==0){
                return value;
            }
            View * view = stack->back();
            view->trackState(this);
            for(View *v: listeners){
                if(v==view){
                    return value;
                }
            }
            listeners.push_back(view);
            return value;
        }

        State<T> & operator=(const T & rhs) {
            checkMutation();
            value = rhs;
            notifyListeners();
            return *this;
        }

        State<T> & operator+=(const T& rhs) {
            checkMutation();
            value += rhs;
            notifyListeners();
            return *this;
        }

        State<T> & operator-=(const T& rhs) {
            checkMutation();
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

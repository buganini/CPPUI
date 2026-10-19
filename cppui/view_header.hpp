#include <iostream>
#include <vector>
#include <algorithm>

#ifndef CPPUI_VIEW_HEADER
#define CPPUI_VIEW_HEADER

#include "node_header.hpp"

namespace CPPUI {
    class View;

    class StateBase {
        public:
        virtual ~StateBase() {}
        virtual void removeListener(View * view) = 0;
    };

    class View: public Node {
        public:
        virtual void content(void) = 0;
        virtual void start(void){};

        std::vector<Node *> frames = std::vector<Node *>();
        std::vector<StateBase *> stateSubscriptions = std::vector<StateBase *>();
        std::vector<StateBase *> statesReadDuringBuild;
        const std::vector<Node *> * previousSizeChildren = nullptr;

        View();
        virtual ~View();

        View & id(const char * name) {
            Node::id(name);
            return *this;
        }
        View & id(const std::string & name) {
            Node::id(name);
            return *this;
        }

        virtual void enter(void);
        virtual void exit(void);

        virtual void update(Node * prev);
        virtual void redraw(void);
        void refreshAfterThaw() override { redraw(); }
        virtual void run(void);

        void trackState(StateBase * state);
        void untrackState(StateBase * state);
    };
    inline void View::trackState(StateBase * state) {
        if(std::find(statesReadDuringBuild.begin(), statesReadDuringBuild.end(), state)
           == statesReadDuringBuild.end()) {
            statesReadDuringBuild.push_back(state);
        }
        if(std::find(stateSubscriptions.begin(), stateSubscriptions.end(), state)
           == stateSubscriptions.end()) {
            stateSubscriptions.push_back(state);
        }
    }

    inline void View::untrackState(StateBase * state) {
        statesReadDuringBuild.erase(
            std::remove(statesReadDuringBuild.begin(), statesReadDuringBuild.end(), state),
            statesReadDuringBuild.end()
        );
        stateSubscriptions.erase(
            std::remove(stateSubscriptions.begin(), stateSubscriptions.end(), state),
            stateSubscriptions.end()
        );
    }


}

#endif

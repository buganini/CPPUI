#include "cppui/state.hpp"
#include "core_headers_support.hpp"

namespace {
    class Leaf: public CPPUI::Node {
        public:
        void destroy(bool) override {}
    };
}

std::vector<CPPUI::View *> * stackFromOtherTranslationUnit() {
    return CPPUI::getStack();
}

int readStateFromOtherTranslationUnit(CPPUI::State<int> & state) {
    return state;
}

void buildNodeFromOtherTranslationUnit() {
    (new Leaf())->id("external-child");
}

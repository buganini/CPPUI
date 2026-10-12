#ifndef CPPUI_TEST_CORE_HEADERS_SUPPORT
#define CPPUI_TEST_CORE_HEADERS_SUPPORT

#include "cppui/state.hpp"

std::vector<CPPUI::View *> * stackFromOtherTranslationUnit();
int readStateFromOtherTranslationUnit(CPPUI::State<int> & state);
void buildNodeFromOtherTranslationUnit();

#endif

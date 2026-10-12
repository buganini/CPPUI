#ifndef CPPUI_COOKBOOK_RECIPES
#define CPPUI_COOKBOOK_RECIPES

#include "cppui/state.hpp"
#include <string>
#include <vector>

namespace Cookbook {
    struct Model {
        CPPUI::State<int> progress{50};
        CPPUI::State<std::string> buttonValue{std::string("A")};
    };
    struct Recipe {
        const char * key;
        const char * title;
        void (*render)(Model &);
        const char * remaining;
    };
    void progressbar(Model &);
    void text(Model &);
    void splitter(Model &);
    const std::vector<Recipe> & recipes();
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id);
    int run(int argc, char ** argv);
}
#endif

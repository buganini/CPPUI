#ifndef CPPUI_COOKBOOK_RECIPES
#define CPPUI_COOKBOOK_RECIPES

#include "cppui/state.hpp"
#include <string>
#include <vector>

namespace Cookbook {
    struct Model {
        CPPUI::State<std::string> buttonValue{std::string("A")};
    };
    struct Recipe {
        const char * key;
        const char * title;
        void (*render)(Model &);
        const char * remaining;
    };
    const std::vector<Recipe> & recipes();
    int run(int argc, char ** argv);
}
#endif

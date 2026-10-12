#ifndef CPPUI_COOKBOOK_RECIPES
#define CPPUI_COOKBOOK_RECIPES

#include "cppui/state.hpp"
#include "cppui/optional.hpp"
#include <string>
#include <vector>
#include <map>

namespace Cookbook {
    struct Model {
        CPPUI::State<bool> checkboxOne{true}, checkboxTwo{true};
        CPPUI::State<std::vector<std::string>> checkboxList{std::vector<std::string>()};
        CPPUI::State<std::map<std::string, bool>> checkboxMap{std::map<std::string, bool>()};
        CPPUI::State<int> progress{50};
        CPPUI::State<int> rotation{15};
        CPPUI::State<std::string> buttonValue{std::string("A")};
        CPPUI::State<bool> modalOpen{false};
        CPPUI::State<CPPUI::OptionalText> directory{CPPUI::OptionalText()};
        CPPUI::State<CPPUI::OptionalText> file{CPPUI::OptionalText()};
        CPPUI::State<CPPUI::Optional<std::vector<std::string>>> files{CPPUI::Optional<std::vector<std::string>>()};
        CPPUI::State<CPPUI::Optional<bool>> confirm{CPPUI::Optional<bool>()};
        CPPUI::State<CPPUI::OptionalText> prompt{CPPUI::OptionalText()};
    };
    struct Recipe {
        const char * key;
        const char * title;
        void (*render)(Model &);
        const char * remaining;
    };
    void checkbox(Model &);
    void progressbar(Model &);
    void text(Model &);
    void splitter(Model &);
    void canvas(Model &);
    void tab(Model &);
    void dialog(Model &);
    void modal(Model &);
    const std::vector<Recipe> & recipes();
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id);
    int run(int argc, char ** argv);
}
#endif

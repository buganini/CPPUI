#ifndef CPPUI_COOKBOOK_RECIPES
#define CPPUI_COOKBOOK_RECIPES

#include "cppui/state.hpp"
#include "cppui/optional.hpp"
#include "adapter_tree_data.hpp"
#include "cppui/tree_adapter.hpp"
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace Cookbook {
    struct HeavyChart;
    struct Model {
        std::shared_ptr<HeavyChart> heavyChart;
        CPPUI::State<bool> rejectDrop{false};
        CPPUI::State<std::string> dropResult{std::string("Drop .txt/.json files, directories (native backends), text, or URLs")}, dropText{std::string()};
        CPPUI::State<int> treeItems{0};
        std::shared_ptr<AdapterTreeData> adapterTree{new AdapterTreeData()};
        std::shared_ptr<CPPUI::BaseTreeAdapter<AdapterTreeNode>> adapterTreeSource;
        using Matrix = std::vector<std::vector<int>>;
        std::shared_ptr<CPPUI::State<Matrix>> adapterTable{new CPPUI::State<Matrix>({
            {4, 9, 2}, {1, 0, 0}, {3, 5, 0}, {3, 3, 2}, {7, 8, 9}})};
        CPPUI::State<std::string> tableA1{std::string("A1")}, tableB1{std::string("B1")}, tableDirection{std::string("CW")};
        CPPUI::State<int> tableSides{4};
        CPPUI::State<std::string> tablePrice{std::string()};
        CPPUI::State<int> scrollRows{100};
        CPPUI::State<int> comboIndex{-1};
        CPPUI::State<std::string> comboText{std::string()}, destination{std::string()}, comboDisabledText{std::string("Value 2")};
        CPPUI::State<std::string> password{std::string()}, bufferedPassword{std::string()}, passwordEditing{std::string()};
        CPPUI::State<std::string> radioValue{std::string()};
        CPPUI::State<std::string> fieldText{std::string("1.23")}, fieldEditing{std::string()};
        CPPUI::State<bool> checkboxOne{true}, checkboxTwo{true}, checkboxDisabled{false};
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
    void drop(Model &);
    void refs(Model &);
    void tree(Model &);
    void treeAdapter(Model &);
    void table(Model &);
    void tableAdapter(Model &);
    void scroll(Model &);
    void combobox(Model &);
    void radiobutton(Model &);
    void textfield(Model &);
    void maskedfield(Model &);
    void grid(Model &);
    void stack(Model &);
    void image(Model &);
    void mdi(Model &);
    void timeline(Model &);
    void checkbox(Model &);
    void progressbar(Model &);
    void text(Model &);
    void splitter(Model &);
    void canvas(Model &);
    void heavycanvas(Model &);
    void tab(Model &);
    void views(Model &);
    void dialog(Model &);
    void modal(Model &);
    const std::vector<Recipe> & recipes();
    CPPUI::Node * find(CPPUI::Node & node, const std::string & id);
    int run(int argc, char ** argv);
}
#endif

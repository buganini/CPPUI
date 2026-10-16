#include "backend.hpp"
#include "recipes.hpp"
#include "cppui/binding.hpp"
#include <algorithm>
#include <sstream>

namespace Cookbook {
    namespace {
        // PUI list/dictionary membership expressed through the public adapter.
        // Assign a modified copy so State notifies its subscribers exactly once.
        CPPUI::Binding<bool> membership(CPPUI::State<std::vector<std::string>> & model, std::string value) {
            return { [&model, value]() {
                const auto items = static_cast<std::vector<std::string>>(model);
                return std::find(items.begin(), items.end(), value) != items.end();
            }, [&model, value](bool checked) {
                auto items = static_cast<std::vector<std::string>>(model);
                auto it = std::find(items.begin(), items.end(), value);
                if(checked && it == items.end()) items.push_back(value);
                if(!checked && it != items.end()) items.erase(it);
                model = items;
            } };
        }
        CPPUI::Binding<bool> membership(CPPUI::State<std::map<std::string, bool>> & model, std::string value) {
            return { [&model, value]() {
                const auto items = static_cast<std::map<std::string, bool>>(model);
                auto it = items.find(value);
                return it != items.end() && it->second;
            }, [&model, value](bool checked) {
                auto items = static_cast<std::map<std::string, bool>>(model);
                if(checked) items[value] = true; else items.erase(value);
                model = items;
            } };
        }
        std::string listText(const std::vector<std::string> & items) {
            std::ostringstream out;
            out << '[';
            for(unsigned i = 0; i < items.size(); ++i) { if(i) out << ", "; out << '\'' << items[i] << '\''; }
            return out.str() + ']';
        }
        std::string mapText(const std::map<std::string, bool> & items) {
            std::ostringstream out;
            out << '{';
            bool first = true;
            for(const auto & item: items) { if(!first) out << ", "; first = false; out << '\'' << item.first << "': " << (item.second ? "True" : "False"); }
            return out.str() + '}';
        }
    }
    void checkbox(Model & model) {
        for(auto column: CPPUI::VBox().layout({}, {}, 1)) {
            CPPUI::Label(std::string("Model value 1: ") + (model.checkboxOne ? "True" : "False"));
            CPPUI::Label(std::string("Model value 2: ") + (model.checkboxTwo ? "True" : "False"));
            CPPUI::Checkbox("checkbox 1-1", model.checkboxOne).id("checkbox-1");
            CPPUI::Checkbox("checkbox 1-2", model.checkboxOne).id("checkbox-1-mirror");
            CPPUI::Checkbox("checkbox 2", model.checkboxTwo).id("checkbox-2");
            CPPUI::Checkbox("Disabled (mirrors value 1)", model.checkboxOne).disabled().id("checkbox-disabled-mirror");
            CPPUI::Checkbox("Disabled (unchecked)", model.checkboxDisabled).disabled().id("checkbox-disabled");
            CPPUI::Divider();
            CPPUI::Label("List value: " + listText(model.checkboxList));
            for(int i = 3; i <= 5; ++i) CPPUI::Checkbox("checkbox " + std::to_string(i),
                membership(model.checkboxList, "value " + std::to_string(i))).id("checkbox-" + std::to_string(i));
            CPPUI::Divider();
            CPPUI::Label("Dict value: " + mapText(model.checkboxMap));
            for(int i = 6; i <= 8; ++i) CPPUI::Checkbox("checkbox " + std::to_string(i),
                membership(model.checkboxMap, "value " + std::to_string(i))).id("checkbox-" + std::to_string(i));
            CPPUI::Spacer();
        }
    }
}

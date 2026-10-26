#include "cppui/backend.hpp"
#include "recipes.hpp"
namespace Cookbook {
    void textedit(Model & model) {
        for(auto box:CPPUI::VBox()) {
            CPPUI::Label("Multiline plain text editor");
            CPPUI::Checkbox("Read only",model.editReadonly);
            CPPUI::Checkbox("Wrap",model.editWrap);
            CPPUI::TextEdit(model.editText,model.editDraft).placeholder("Type several lines...")
                .readonly(model.editReadonly).wrap(model.editWrap)
                .input([&model]() {model.editStatus="Editing "+std::to_string(std::string(model.editDraft).size())+" bytes";})
                .change([&model]() {model.editStatus="Committed "+std::to_string(std::string(model.editText).size())+" bytes";})
                .layout(CPPUI::layout::weight(1)).id("multiline-editor");
            CPPUI::Label(model.editStatus);
            CPPUI::Text("Committed: "+std::string(model.editText));
            CPPUI::Text("Draft: "+std::string(model.editDraft));
        }
    }
}

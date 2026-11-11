#ifndef CPPUI_LVGL_FONTCOMBOBOX
#define CPPUI_LVGL_FONTCOMBOBOX
#include "combobox.hpp"
#include "../font_choices.hpp"
namespace CPPUI { namespace LVGL {
    class FontComboBox: public ComboBox {
    public:
        FontComboBox(Binding<std::string> model,const std::vector<std::string> & fonts):ComboBox(false,{},model) {
            auto scope=begin();
            for(const auto & name:fonts) new ComboBoxItem(name,name);
        }
    };
}}
namespace CPPUI {
    inline LVGL::FontComboBox & FontComboBox(Binding<std::string> model,const std::vector<std::string> & fonts) {
        return *new LVGL::FontComboBox(model,detail::fontChoices(fonts));
    }
    inline LVGL::FontComboBox & FontComboBox(Binding<std::string> model) {
        return FontComboBox(model,{"sans-serif","serif","monospace"});
    }
}
#endif

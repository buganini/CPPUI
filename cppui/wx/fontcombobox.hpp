#ifndef CPPUI_WX_FONTCOMBOBOX
#define CPPUI_WX_FONTCOMBOBOX
#include "combobox.hpp"
#include "../font_choices.hpp"
#include <wx/fontenum.h>
namespace CPPUI { namespace Wx {
    class FontComboBox: public ComboBox {
    public:
        FontComboBox(Binding<std::string> model,const std::vector<std::string> & fonts):ComboBox(false,{},model) {
            auto scope=begin();
            for(const auto & name:fonts) new ComboBoxItem(name,name);
        }
    };
}}
namespace CPPUI {
    inline Wx::FontComboBox & FontComboBox(Binding<std::string> model,const std::vector<std::string> & fonts) {
        return *new Wx::FontComboBox(model,detail::fontChoices(fonts));
    }
    inline Wx::FontComboBox & FontComboBox(Binding<std::string> model) {
        std::vector<std::string> fonts;
        for(const auto & name:wxFontEnumerator::GetFacenames()) fonts.push_back(name.ToStdString(wxConvUTF8));
        return FontComboBox(model,fonts);
    }
}
#endif

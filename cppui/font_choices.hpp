#ifndef CPPUI_FONT_CHOICES
#define CPPUI_FONT_CHOICES
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>
namespace CPPUI { namespace detail {
    inline std::vector<std::string> fontChoices(const std::vector<std::string> & fonts) {
        std::vector<std::string> result;
        for(const auto & name:fonts) {
            if(name.empty() || std::all_of(name.begin(),name.end(),[](unsigned char c){return std::isspace(c);}))
                throw std::invalid_argument("Font family names must be nonempty strings");
            if(std::find(result.begin(),result.end(),name)==result.end()) result.push_back(name);
        }
        return result;
    }
}}
#endif

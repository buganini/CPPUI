#ifndef CPPUI_WX_URL
#define CPPUI_WX_URL
#include "../url.hpp"
#include <wx/utils.h>
namespace CPPUI {
    inline void OpenURL(const std::string & url) {
        if(!wxLaunchDefaultBrowser(wxString::FromUTF8(detail::validateURL(url)),wxBROWSER_NEW_WINDOW))
            throw std::runtime_error("No browser accepted the URL");
    }
}
#endif

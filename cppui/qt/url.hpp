#ifndef CPPUI_QT_URL
#define CPPUI_QT_URL
#include "../url.hpp"
#include <QDesktopServices>
#include <QUrl>
namespace CPPUI {
    inline void OpenURL(const std::string & url) {
        if(!QDesktopServices::openUrl(QUrl(QString::fromStdString(detail::validateURL(url)))))
            throw std::runtime_error("No browser accepted the URL");
    }
}
#endif

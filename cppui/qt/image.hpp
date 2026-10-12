#ifndef CPPUI_QT_IMAGE
#define CPPUI_QT_IMAGE

#include "label.hpp"
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>

namespace CPPUI { namespace Qt {
    class Image: public Label {
        std::string path;
        QDateTime modified;
        protected:
        QPixmap pixmap;
        public:
        explicit Image(const std::string & path): Label(""), path(path) {
            QFileInfo file(QString::fromStdString(path));
            if(!file.exists()) throw std::runtime_error("Image file does not exist: " + path);
            modified = file.lastModified();
        }
        void update(BaseWidget * previous) override {
            auto prev = static_cast<Image *>(previous);
            if(prev && prev->path == path && prev->modified == modified) pixmap = prev->pixmap;
            else pixmap.load(QString::fromStdString(path));
            Label::update(previous);
            auto label = static_cast<QLabel *>(ui);
            label->setScaledContents(false);
            label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            label->setPixmap(pixmap);
        }
    };
}}
namespace CPPUI {
    inline Qt::Image & Image(const std::string & path) { return *new Qt::Image(path); }
}
#endif

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
            const bool stretch = layout_weight && *layout_weight > 0;
            label->setScaledContents(stretch);
            label->setSizePolicy(stretch ? QSizePolicy::Ignored : QSizePolicy::Preferred,
                                 stretch ? QSizePolicy::Ignored : QSizePolicy::Preferred);
            auto displayed = pixmap;
            if(layout_width && layout_height) displayed = pixmap.scaled(*layout_width, *layout_height, ::Qt::KeepAspectRatio, ::Qt::SmoothTransformation);
            else if(layout_width) displayed = pixmap.scaledToWidth(*layout_width, ::Qt::SmoothTransformation);
            else if(layout_height) displayed = pixmap.scaledToHeight(*layout_height, ::Qt::SmoothTransformation);
            label->setPixmap(displayed);
        }
    };
}}
namespace CPPUI {
    inline Qt::Image & Image(const std::string & path) { return *new Qt::Image(path); }
}
#endif

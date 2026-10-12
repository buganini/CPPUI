#ifndef CPPUI_QT_IMAGE_RESOURCE
#define CPPUI_QT_IMAGE_RESOURCE

#include <QImage>
#include <string>
#include <utility>

namespace CPPUI {
    class CanvasPainter;

    class ImageResource {
        QImage image;
        explicit ImageResource(QImage image): image(std::move(image)) {}
        friend class CanvasPainter;

        public:
        ImageResource() = default;
        static ImageResource load(const std::string & path) {
            return ImageResource(QImage(QString::fromStdString(path)));
        }
        ImageResource crop(int x, int y, int width, int height) const {
            return ImageResource(image.copy(x, y, width, height));
        }
        ImageResource scale(int width, int height, bool keepAspectRatio = true, int quality = 0) const {
            return ImageResource(image.scaled(width, height,
                keepAspectRatio ? ::Qt::KeepAspectRatio : ::Qt::IgnoreAspectRatio,
                quality == 0 ? ::Qt::FastTransformation : ::Qt::SmoothTransformation));
        }
        bool isNull() const { return image.isNull(); }
        int width() const { return image.width(); }
        int height() const { return image.height(); }
    };
}
#endif

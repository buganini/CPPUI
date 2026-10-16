#include "cppui/qt/canvas.hpp"
#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QTemporaryDir>
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    void drawingAndImages() {
        QTemporaryDir directory;
        require(directory.isValid(), "temporary image directory unavailable");
        const auto path = directory.filePath("source.png");
        QImage source(8, 4, QImage::Format_ARGB32);
        source.fill(Qt::red);
        for(int y = 0; y < 4; ++y) for(int x = 4; x < 8; ++x) source.setPixelColor(x, y, Qt::green);
        require(source.save(path), "test image could not be saved");
        auto image = CPPUI::ImageResource::load(path.toStdString());
        require(!image.isNull() && image.width() == 8 && image.height() == 4, "image loading lost dimensions");
        require(QFile::remove(path), "test image could not be removed");
        auto cropped = image.crop(4, 0, 4, 4);
        auto scaled = image.scale(16, 16);
        auto stretched = image.scale(16, 16, false, 1);
        require(cropped.width() == 4 && cropped.height() == 4
            && scaled.width() == 16 && scaled.height() == 8
            && stretched.width() == 16 && stretched.height() == 16,
            "crop/aspect-preserving/stretch scaling differs from reference");
        auto missing = CPPUI::ImageResource::load(path.toStdString());
        require(missing.isNull(), "missing image should remain an empty resource");

        QImage pixels(160, 140, QImage::Format_ARGB32);
        pixels.fill(Qt::white);
        {
            QPainter native(&pixels);
            native.setPen(QPen(Qt::black, 3));
            native.setBrush(Qt::darkGray);
            const auto initialPen = native.pen();
            const auto initialBrush = native.brush();
            const auto initialFont = native.font();
            const auto initialTransform = native.transform();
            {
            CPPUI::CanvasPainter canvas(native, pixels.width(), pixels.height(), 1);
            canvas.drawRect(30, 30, 10, 10, 0xFF0000);
            canvas.drawLine(40, 10, 80, 10, "#00ff00", 2);
            canvas.drawPolyline({{40, 20}, {60, 20}, {60, 30}}, 0xFF00FF, 2);
            canvas.drawCubicBezier({40, 40}, {50, 40}, {70, 40}, {80, 40}, 0x00FFFF, 2);
            canvas.drawPolygon({{10, 45}, {30, 45}, {20, 60}}, 0xFFFF00);
            canvas.drawEllipse(60, 60, 10, 5, 0x0000FF);
            canvas.drawText(0, 80, "Canvas", -1, -1, 12, 0x000000, 0, CPPUI::Anchor::CENTER);
            canvas.drawText(90, 75, "Rotate", 60, 60, 12, 0x000000, 45, CPPUI::Anchor::RIGHT_BOTTOM);
            canvas.drawImage(cropped, 100, 10, 8, 8);
            canvas.drawImage(image, 110, 30, -1, -1, 4, 0, -1, -1);
            canvas.drawImage(image, 100, 50, 8, 8, 0, 0, 4, 4, 0.5);
            canvas.drawRect(100, 65, 108, 73, 0x0000FF);
            canvas.drawImage(missing, 0, 0, 160, 140);
            canvas.drawLine(130, 10, 150, 10); // Missing color means no stroke.
            canvas.drawRect(130, 20, 150, 40); // Missing fill/stroke means no paint.
            canvas.drawPolyline({}, 0xFF0000);
            canvas.drawPolygon({}, 0xFF0000);
            }
            require(native.pen() == initialPen && native.brush() == initialBrush
                && native.font() == initialFont && native.transform() == initialTransform
                && native.opacity() == 1, "paint context did not restore native state");
        }
        require(pixels.pixelColor(20, 20) == QColor(Qt::red), "reversed rectangle corners did not normalize");
        require(pixels.pixelColor(60, 10) == QColor(Qt::green), "line color or position is wrong");
        require(pixels.pixelColor(50, 20) == QColor(Qt::magenta), "polyline did not render");
        require(pixels.pixelColor(60, 40) == QColor(Qt::cyan), "cubic Bezier did not render");
        require(pixels.pixelColor(20, 50) == QColor(Qt::yellow), "polygon fill did not render");
        require(pixels.pixelColor(60, 60) == QColor(Qt::blue), "ellipse fill did not render");
        require(pixels.pixelColor(104, 14) == QColor(Qt::green)
            && pixels.pixelColor(112, 32) == QColor(Qt::green), "image crop/source region was lost");
        const auto blend = pixels.pixelColor(104, 54);
        require(blend.red() == 255 && blend.green() >= 126 && blend.green() <= 129
            && blend.blue() == blend.green(), "image opacity did not blend with the destination");
        require(pixels.pixelColor(104, 69) == QColor(Qt::blue), "image opacity affected later primitives");
        require(pixels.pixelColor(140, 10) == QColor(Qt::white)
            && pixels.pixelColor(140, 30) == QColor(Qt::white), "omitted colors unexpectedly painted");
        int textPixels = 0;
        for(int y = 80; y < 140; ++y) for(int x = 0; x < 90; ++x) {
            if(pixels.pixelColor(x, y).value() < 128) ++textPixels;
        }
        require(textPixels > 20, "text did not render");
    }

    void frameStyles() {
        using Draw = std::function<void(CPPUI::CanvasPainter &)>;
        const std::vector<Draw> operations{
            [](CPPUI::CanvasPainter & c){c.drawRect(4,4,40,30,0xff0000,0x224466,3);},
            [](CPPUI::CanvasPainter & c){c.drawLine(45,15,90,20,0x00ff00,4);},
            [](CPPUI::CanvasPainter & c){c.drawRect(100,4,130,30);},
            [](CPPUI::CanvasPainter & c){c.drawText(10,40,"Default after NoPen",-1,-1,-1);},
            [](CPPUI::CanvasPainter & c){c.drawText(10,80,"Rotated",-1,-1,12,0xff0000,15);},
            [](CPPUI::CanvasPainter & c){c.drawText(10,110,"Default again",-1,-1,-1);}
        };
        auto render = [&](bool isolate, QColor foreground) {
            QImage image(240,150,QImage::Format_ARGB32);image.fill(Qt::white);
            QPainter painter(&image);QPen pen(foreground,2);pen.setCapStyle(Qt::RoundCap);pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);painter.setBrush(Qt::cyan);
            { CPPUI::CanvasPainter canvas(painter,240,150,1);
              for(const auto & op:operations) {
                  if(isolate) painter.save();
                  op(canvas);
                  if(isolate) painter.restore();
              }
            }
            painter.end();return image;
        };
        for(auto foreground:{QColor(Qt::black),QColor(Qt::blue)})
            require(render(false,foreground)==render(true,foreground),"primitive styles leaked into later text/shapes");
        require(render(false,Qt::black)!=render(false,Qt::blue),"new frame retained the previous default pen");
        QImage image(20,20,QImage::Format_ARGB32);QPainter painter(&image);const auto before=painter.pen();
        try {CPPUI::CanvasPainter canvas(painter,20,20,1);canvas.drawLine(0,0,10,10,0xff0000);throw std::runtime_error("expected");}
        catch(const std::runtime_error &) {}
        require(painter.pen()==before,"failed frame did not restore native state");
    }

    int captures = 0;
    class InputRoot: public CPPUI::Qt::QtView {
        public:
        bool show=true, enabled=true, remove=false;
        int version=1, calls=0;
        CPPUI::CanvasPointerEvent last;
        void content() override {
            if(!show) return;
            auto & canvas=CPPUI::Canvas([](CPPUI::CanvasPainter &){});
            if(enabled) {
                const int current=version;
                canvas.mousemove([this,current](const CPPUI::CanvasPointerEvent & event){calls+=current;last=event;})
                    .mouseleave([this,current](){calls+=current;})
                    .wheel([this,current](const CPPUI::CanvasPointerEvent & event){calls+=current;last=event;if(remove){show=false;sync();}});
            }
        }
    };
    void pointerInput() {
        InputRoot root; root.sync(); auto widget=static_cast<QWidget *>(root.children->front()->outer());
        for(int i=1;i<=3;++i) {
            root.version=i;root.sync();require(root.children->front()->outer()==widget,"Canvas input redraw remounted native widget");
            const int before=root.calls;
            QMouseEvent move(QEvent::MouseMove,QPointF(12,34),QPointF(12,34),Qt::NoButton,Qt::NoButton,Qt::NoModifier);
            QCoreApplication::sendEvent(widget,&move);
            require(root.last.x==12 && root.last.y==34,"Canvas move lost local coordinates");
            QWheelEvent wheel(QPointF(20,30),QPointF(20,30),QPoint(0,7),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
            QCoreApplication::sendEvent(widget,&wheel);
            require(root.last.angleY==120 && root.last.pixelY==7,"Canvas mixed wheel angle/pixel units");
            QEvent leave(QEvent::Leave);QCoreApplication::sendEvent(widget,&leave);
            require(root.calls==before+3*i,"Canvas dispatched stale/duplicate input callbacks");
        }
        root.enabled=false;root.sync();int before=root.calls;
        QEvent leave(QEvent::Leave);QCoreApplication::sendEvent(widget,&leave);
        require(root.calls==before && !widget->hasMouseTracking(),"omitted Canvas input callbacks retained dispatch");
        root.enabled=true;root.remove=true;root.sync();
        QWheelEvent wheel(QPointF(20,30),QPointF(20,30),QPoint(),QPoint(0,120),Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QCoreApplication::sendEvent(widget,&wheel);require(root.children->empty(),"Canvas input self-removal failed");
        QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    }
    struct Capture { Capture() { ++captures; } ~Capture() { --captures; } };
    class VoidPainterRoot: public CPPUI::Qt::QtView {
        public:
        int paints = 0;
        void content() override {
            CPPUI::Canvas([this](CPPUI::CanvasPainter & canvas) {
                ++paints;
                canvas.drawText(0, 0, "Native font", -1, -1, -1);
            });
        }
    };
    class Root: public CPPUI::Qt::QtView {
        public:
        bool show = true;
        bool removeDuringPaint = false;
        bool redrawDuringPaint = false;
        bool finished = false;
        int version = 1;
        int seenVersion = 0;
        int paints = 0;
        int repeat = 0;
        int observedWidth = 0;
        int observedHeight = 0;
        double density = 0;
        ~Root() {
            clearChildren();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }
        void content() override {
            if(!show) return;
            const int current = version;
            auto capture = std::make_shared<Capture>();
            CPPUI::Canvas([this, current, capture](CPPUI::CanvasPainter & canvas) {
                ++paints;
                seenVersion = current;
                observedWidth = canvas.width;
                observedHeight = canvas.height;
                density = canvas.pixel_density;
                if(redrawDuringPaint) {
                    redrawDuringPaint = false;
                    ++version;
                    sync();
                }
                if(removeDuringPaint) {
                    removeDuringPaint = false;
                    show = false;
                    sync();
                }
                // Drawing remains valid after the callback's declaration retires.
                canvas.drawRect(10, 10, 30, 30, current == 1 ? 0xFF0000 : 0x0000FF);
                finished = true;
                if(repeat > 0) { --repeat; return true; }
                return false;
            }).id("canvas");
        }
        QWidget * native() { return static_cast<QWidget *>(children->front()->outer()); }
    };
    QImage render(QWidget * widget) {
        const auto ratio = widget->devicePixelRatioF();
        QImage image(widget->size() * ratio, QImage::Format_ARGB32);
        image.setDevicePixelRatio(ratio);
        image.fill(Qt::white);
        widget->render(&image);
        return image;
    }
    void nativeLifetime(QApplication & app) {
        {
            VoidPainterRoot root;
            root.sync();
            auto widget = static_cast<QWidget *>(root.children->front()->outer());
            widget->resize(120, 100);
            render(widget);
            require(root.paints == 1, "void paint callback was not dispatched");
        }
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        Root root;
        root.sync();
        auto widget = root.native();
        widget->resize(120, 100);
        render(widget);
        require(root.observedWidth == 120 && root.observedHeight == 100
            && root.density == std::max(1.0, widget->devicePixelRatioF()), "paint context lost logical dimensions/density");
        require(root.seenVersion == 1 && captures == 1, "initial painter did not own its capture");
        root.version = 2;
        root.sync();
        require(root.native() == widget && captures == 1, "painter replacement leaked a capture or replaced the widget");
        render(widget);
        require(root.seenVersion == 2, "native paint dispatched to a retired callback");
        root.redrawDuringPaint = true;
        render(widget);
        require(root.finished && captures == 1, "redraw invalidated active paint or retained an old capture");
        render(widget);
        require(root.seenVersion == 3, "redraw during paint failed to install the next callback");
        root.removeDuringPaint = true;
        root.finished = false;
        render(widget);
        require(root.finished && captures == 0 && root.children->empty(), "self-removal invalidated paint or retained captures");
        const int before = root.paints;
        render(widget); // Native deletion is deferred; removed dispatch must be inert.
        require(root.paints == before, "removed canvas still dispatched its callback");
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        root.show = true;
        root.repeat = 2;
        root.sync();
        widget = root.native();
        widget->resize(120, 100);
        widget->show();
        for(int i = 0; i < 8; ++i) app.processEvents();
        require(root.repeat == 0 && root.paints >= before + 3, "true did not request another native paint");
        const int settled = root.paints;
        for(int i = 0; i < 4; ++i) app.processEvents();
        require(root.paints == settled, "false failed to stop repeat painting");
        widget->update();
        root.show = false;
        root.sync();
        app.processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        require(root.paints == settled && captures == 0, "queued paint reached a removed declaration");
        root.show = true;
        root.sync();
        delete root.native();
        require(!root.children->front()->outer(), "native deletion left a stale canvas observer");
    }
}
int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        drawingAndImages();
        frameStyles();
        pointerInput();
        nativeLifetime(app);
        require(captures == 0, "root teardown retained a paint capture");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

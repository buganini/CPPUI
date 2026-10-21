#ifndef CPPUI_QT_CANVAS
#define CPPUI_QT_CANVAS

#include "base.hpp"
#include "../anchor.hpp"
#include "../canvas_input.hpp"
#include <QWheelEvent>
#include <QLabel>
#include "../frame_rate.hpp"
#include "image_resource.hpp"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace CPPUI {

    class CanvasColor {
        QColor color;
        bool present = false;
        friend class CanvasPainter;
        public:
        CanvasColor() = default;
        CanvasColor(int rgb): CanvasColor(static_cast<std::uint32_t>(rgb)) {}
        CanvasColor(std::uint32_t rgb): color(QColor::fromRgb(rgb)), present(true) {}
        CanvasColor(const char * name): color(QString::fromUtf8(name)), present(true) {}
        CanvasColor(const std::string & name): color(QString::fromStdString(name)), present(true) {}
    };

    struct CanvasPoint { double x; double y; };

    // Valid only during the paint callback. It owns no declaration/native widget.
    class CanvasPainter {
        QPainter & painter;
        class SavedState {
            QPainter & painter;
            public:
            explicit SavedState(QPainter & painter): painter(painter) { painter.save(); }
            ~SavedState() { painter.restore(); }
        };
        SavedState frame;
        std::size_t scopeDepth = 0;
        const QPen defaultPen;
        const QBrush defaultBrush;
        using ColorKey = std::tuple<bool, bool, quint64>;
        std::map<std::pair<ColorKey, int>, QPen> pens;
        std::map<ColorKey, QBrush> brushes;
        static ColorKey key(CanvasColor color) {
            return std::make_tuple(color.present, color.color.isValid(), quint64(color.color.rgba64()));
        }
        void pen(CanvasColor color, int width) {
            const auto identity = std::make_pair(key(color), width);
            auto found = pens.find(identity);
            if(found == pens.end()) {
                auto value = defaultPen;
                value.setStyle(color.present ? ::Qt::SolidLine : ::Qt::NoPen);
                if(color.present) value.setColor(color.color);
                value.setWidth(width);
                found = pens.emplace(identity, value).first;
            }
            painter.setPen(found->second);
        }
        void brush(CanvasColor color) {
            const auto identity = key(color);
            auto found = brushes.find(identity);
            if(found == brushes.end()) {
                auto value = defaultBrush;
                value.setStyle(color.present ? ::Qt::SolidPattern : ::Qt::NoBrush);
                if(color.present) value.setColor(color.color);
                found = brushes.emplace(identity, value).first;
            }
            painter.setBrush(found->second);
        }
        static QPolygonF points(const std::vector<CanvasPoint> & coordinates) {
            QPolygonF result;
            for(const auto & point: coordinates) result << QPointF(point.x, point.y);
            return result;
        }

        public:
        const int width;
        const int height;
        const double pixel_density;
        CanvasPainter(QPainter & painter, int width, int height, double density)
            : painter(painter), frame(painter), defaultPen(painter.pen()), defaultBrush(painter.brush()),
              width(width), height(height), pixel_density(density) {}
        ~CanvasPainter() { while(scopeDepth) { painter.restore(); --scopeDepth; } }
        void push() { painter.save(); ++scopeDepth; }
        void pop() {
            if(!scopeDepth) throw std::logic_error("Canvas.pop() has no matching push()");
            painter.restore(); --scopeDepth;
        }
        CanvasPainter(const CanvasPainter &) = delete;
        CanvasPainter & operator=(const CanvasPainter &) = delete;

        void drawText(double x, double y, const std::string & text, int w = -1, int h = -1,
                      int size = 12, CanvasColor color = {}, double rotate = 0, Anchor anchor = Anchor::LeftTop) {
            SavedState saved(painter);
            if(w < 0) w = width;
            if(h < 0) h = height;
            painter.setPen(defaultPen);
            const auto string = QString::fromStdString(text);
            // Match PUI: anchor selects the rotation pivot within the text box.
            const auto bounds = painter.boundingRect(QRect(0, 0, w, h), 0, string);
            const int horizontal = static_cast<int>(anchor) / 3;
            const int vertical = static_cast<int>(anchor) % 3;
            const double dx = bounds.width() * horizontal / 2.0;
            const double dy = bounds.height() * vertical / 2.0;
            painter.translate(static_cast<int>(x + dx), static_cast<int>(y + dy));
            painter.rotate(rotate);
            painter.translate(static_cast<int>(-dx), static_cast<int>(-dy));
            if(size >= 0) {
                auto font = painter.font();
                font.setPointSize(size);
                painter.setFont(font);
            }
            if(color.present) painter.setPen(color.color);
            painter.drawText(QRect(0, 0, w, h), 0, string);
        }
        void drawLine(double x1, double y1, double x2, double y2, CanvasColor color = {}, int width = 1) {
            pen(color, width);
            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));
        }
        void drawPolyline(const std::vector<CanvasPoint> & coordinates, CanvasColor color = {}, int width = 1) {
            pen(color, width);
            painter.drawPolyline(points(coordinates));
        }
        void drawCubicBezier(CanvasPoint start, CanvasPoint control1, CanvasPoint control2, CanvasPoint end,
                             CanvasColor color = {}, int width = 1) {
            brush({});
            pen(color, width);
            QPainterPath path(QPointF(start.x, start.y));
            path.cubicTo(control1.x, control1.y, control2.x, control2.y, end.x, end.y);
            painter.drawPath(path);
        }
        void drawPolygon(const std::vector<CanvasPoint> & coordinates, CanvasColor fill = {},
                         CanvasColor stroke = {}, int width = 1) {
            brush(fill);
            pen(stroke, width);
            painter.drawPolygon(points(coordinates));
        }
        void drawRect(double x1, double y1, double x2, double y2, CanvasColor fill = {},
                      CanvasColor stroke = {}, int width = 1) {
            brush(fill);
            pen(stroke, width);
            painter.drawRect(QRectF(QPointF(x1, y1), QPointF(x2, y2)).normalized());
        }
        void drawEllipse(double x, double y, double rx, double ry, CanvasColor fill = {},
                         CanvasColor stroke = {}, int width = 1) {
            brush(fill);
            pen(stroke, width);
            painter.drawEllipse(QPointF(x, y), rx, ry);
        }
        ImageResource loadImage(const std::string & path) const { return ImageResource::load(path); }
        void drawImage(const ImageResource & image, int x = 0, int y = 0, int width = -1, int height = -1,
                       int src_x = 0, int src_y = 0, int src_width = -1, int src_height = -1, double opacity = 1) {
            if(image.isNull()) return;
            SavedState saved(painter);
            if(src_width < 0) src_width = image.width() - src_x;
            if(src_height < 0) src_height = image.height() - src_y;
            if(width < 0) width = src_width;
            if(height < 0) height = src_height;
            painter.setOpacity(opacity);
            painter.drawImage(QRect(x, y, width, height), image.image,
                              QRect(src_x, src_y, src_width, src_height));
        }
    };

    namespace Qt {
        class CanvasWidget: public Widget<QWidget> {
            public:
            using Painter = std::function<bool(CanvasPainter &)>;
            std::function<void(const CanvasPointerEvent &)> moved, wheeled;
            std::function<void()> left;
            private:
            Painter painter;
            bool showFPS = false;
            FrameRate fps;
            QLabel * fpsLabel = nullptr;
            std::uint64_t generation = 0;
            protected:
            void mouseMoveEvent(QMouseEvent * event) override {
                auto invoke = moved;
                if(!invoke) { Widget<QWidget>::mouseMoveEvent(event); return; }
                CanvasPointerEvent value; value.x=event->position().x(); value.y=event->position().y();
                try { invoke(value); } catch(const std::exception & error) { std::cerr << "CPPUI Canvas move: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI Canvas move failed\n"; }
            }
            void leaveEvent(QEvent * event) override {
                auto invoke = left;
                if(!invoke) { Widget<QWidget>::leaveEvent(event); return; }
                try { invoke(); } catch(const std::exception & error) { std::cerr << "CPPUI Canvas leave: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI Canvas leave failed\n"; }
            }
            void wheelEvent(QWheelEvent * event) override {
                auto invoke = wheeled;
                if(!invoke) { Widget<QWidget>::wheelEvent(event); return; }
                CanvasPointerEvent value; value.x=event->position().x(); value.y=event->position().y();
                value.angleX=event->angleDelta().x(); value.angleY=event->angleDelta().y();
                value.pixelX=event->pixelDelta().x(); value.pixelY=event->pixelDelta().y();
                event->accept();
                try { invoke(value); } catch(const std::exception & error) { std::cerr << "CPPUI Canvas wheel: " << error.what() << '\n'; }
                catch(...) { std::cerr << "CPPUI Canvas wheel failed\n"; }
            }
            void paintEvent(QPaintEvent *) override {
                // A redraw can reclaim the declaration while its painter runs.
                auto invoke = painter;
                if(!invoke) return;
                const auto version = generation;
                bool again;
                {
                    QPainter native(this);
                    native.setRenderHint(QPainter::Antialiasing);
                    CanvasPainter canvas(native, width(), height(), std::max(1.0, devicePixelRatioF()));
                    again = invoke(canvas);
                }
                if(showFPS && painter && version == generation) {
                    if(!fpsLabel) {
                        fpsLabel = new QLabel(this);
                        fpsLabel->setObjectName("cppui-fps");
                        fpsLabel->setStyleSheet("background: #101a2b; color: white; padding: 4px;");
                        fpsLabel->setAttribute(::Qt::WA_TransparentForMouseEvents);
                    }
                    if(auto text = fps.tick()) { fpsLabel->setText(QString::fromStdString(*text)); fpsLabel->adjustSize(); }
                    fpsLabel->move(std::max(0, width() - fpsLabel->width() - 8), 8);
                    fpsLabel->show();
                }
                if(again && painter && version == generation) update();
            }
            public:
            void setShowFPS(bool value) {
                if(showFPS == value) return;
                showFPS = value; fps.reset();
                if(!value) { delete fpsLabel; fpsLabel = nullptr; }
            }
            void setPainter(Painter callback) { painter = std::move(callback); ++generation; update(); }
            void clearPainter() { painter = {}; moved = {}; wheeled = {}; left = {}; ++generation; }
        };

        class Canvas: public BaseWidget {
            public:
            using Painter = CanvasWidget::Painter;
            bool showFPS = false;
            Canvas & showFps(bool value = true) { showFPS = value; return *this; }
            Painter painter;
            std::function<void(const CanvasPointerEvent &)> moved, wheeled;
            std::function<void()> left;
            Canvas & mousemove(std::function<void(const CanvasPointerEvent &)> callback) { moved=std::move(callback); return *this; }
            Canvas & mouseleave(std::function<void()> callback) { left=std::move(callback); return *this; }
            Canvas & wheel(std::function<void(const CanvasPointerEvent &)> callback) { wheeled=std::move(callback); return *this; }
            explicit Canvas(Painter painter): painter(std::move(painter)) { is_terminal = true; expandX=expandY=3; }
            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new CanvasWidget();
                auto canvas = static_cast<CanvasWidget *>(ui);
                canvas->moved=moved; canvas->wheeled=wheeled; canvas->left=left;
                canvas->setMouseTracking(bool(moved));
                canvas->setShowFPS(showFPS);
                canvas->setPainter(painter);
            }
            void destroy(bool direct) override {
                if(ui) static_cast<CanvasWidget *>(ui)->clearPainter();
                BaseWidget::destroy(direct);
            }
        };

        namespace detail {
            template<class Callback>
            Canvas::Painter canvasCallback(Callback callback, std::true_type) {
                return [callback](CanvasPainter & canvas) mutable { callback(canvas); return false; };
            }
            template<class Callback>
            Canvas::Painter canvasCallback(Callback callback, std::false_type) {
                return [callback](CanvasPainter & canvas) mutable { return static_cast<bool>(callback(canvas)); };
            }
        }
    }
    template<class Callback>
    Qt::Canvas & Canvas(Callback callback) {
        using Result = typename std::result_of<Callback &(CanvasPainter &)>::type;
        return *new Qt::Canvas(Qt::detail::canvasCallback(std::move(callback), std::is_void<Result>()));
    }
}
#endif

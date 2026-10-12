#ifndef CPPUI_LVGL_CANVAS
#define CPPUI_LVGL_CANVAS

#include "base.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace CPPUI {
    enum class Anchor {
        LEFT_TOP, LEFT_CENTER, LEFT_BOTTOM,
        CENTER_TOP, CENTER, CENTER_BOTTOM,
        RIGHT_TOP, RIGHT_CENTER, RIGHT_BOTTOM
    };
    struct CanvasPoint { double x; double y; };

    class CanvasColor {
        lv_color_t color = lv_color_black();
        lv_opa_t opacity = LV_OPA_COVER;
        bool present = false;
        friend class CanvasPainter;
        public:
        CanvasColor() = default;
        CanvasColor(int rgb): CanvasColor(static_cast<std::uint32_t>(rgb)) {}
        CanvasColor(std::uint32_t rgb): color(lv_color_hex(rgb)), present(true) {}
        CanvasColor(const char * name): CanvasColor(std::string(name ? name : "")) {}
        CanvasColor(const std::string & name) {
            std::string value = name;
            for(auto & c: value) if(c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if(value.size() == 4 || value.size() == 7 || value.size() == 9) {
                if(value[0] == '#') {
                    std::uint32_t rgb = 0;
                    for(std::size_t i = 1; i < value.size(); ++i) {
                        const char c = value[i];
                        const int digit = c >= '0' && c <= '9' ? c - '0' :
                                          c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
                        if(digit < 0) throw std::invalid_argument("invalid Canvas color: " + name);
                        rgb = (rgb << 4) | digit;
                    }
                    if(value.size() == 4) rgb = ((rgb & 0xf00) << 8) | ((rgb & 0xf00) << 4)
                        | ((rgb & 0xf0) << 8) | ((rgb & 0xf0) << 4) | ((rgb & 0xf) << 4) | (rgb & 0xf);
                    if(value.size() == 9) opacity = static_cast<lv_opa_t>(rgb >> 24);
                    color = lv_color_hex(rgb);
                    present = true;
                    return;
                }
            }
            const struct { const char * name; std::uint32_t rgb; } names[] = {
                {"black", 0x000000}, {"silver", 0xc0c0c0}, {"gray", 0x808080}, {"grey", 0x808080},
                {"white", 0xffffff}, {"maroon", 0x800000}, {"red", 0xff0000}, {"purple", 0x800080},
                {"fuchsia", 0xff00ff}, {"magenta", 0xff00ff}, {"green", 0x008000}, {"lime", 0x00ff00},
                {"olive", 0x808000}, {"yellow", 0xffff00}, {"navy", 0x000080}, {"blue", 0x0000ff},
                {"teal", 0x008080}, {"aqua", 0x00ffff}, {"cyan", 0x00ffff}, {"orange", 0xffa500}
            };
            for(const auto & entry: names) if(value == entry.name) {
                color = lv_color_hex(entry.rgb);
                present = true;
                return;
            }
            if(value == "transparent") { present = true; opacity = LV_OPA_TRANSP; return; }
            throw std::invalid_argument("unsupported LVGL Canvas color: " + name);
        }
    };

    namespace LVGL { class Canvas; namespace detail {
        struct CanvasBufferDeleter {
            void operator()(lv_draw_buf_t * buffer) const {
                // Uncompressed, aligned ARGB draw buffers are used directly by LVGL's decoder.
                lv_draw_buf_destroy(buffer);
            }
        };
        using CanvasBuffer = std::unique_ptr<lv_draw_buf_t, CanvasBufferDeleter>;
        inline CanvasBuffer canvasBuffer(int width, int height) {
            CanvasBuffer buffer(lv_draw_buf_create(width, height, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO));
            if(!buffer) throw std::bad_alloc();
            return buffer;
        }
        class CanvasLayer {
            lv_obj_t * object;
            public:
            lv_layer_t layer;
            explicit CanvasLayer(lv_obj_t * object): object(object) { lv_canvas_init_layer(object, &layer); }
            ~CanvasLayer() { lv_canvas_finish_layer(object, &layer); }
            CanvasLayer(const CanvasLayer &) = delete;
            CanvasLayer & operator=(const CanvasLayer &) = delete;
        };
    }}

    // Records one frame; no native object/declaration is borrowed by the callback.
    class CanvasPainter {
        using Command = std::function<void(lv_obj_t *)>;
        std::vector<Command> commands;
        const lv_font_t * font;
        friend class LVGL::Canvas;

        static void line(lv_layer_t & layer, CanvasPoint a, CanvasPoint b, CanvasColor color, int width) {
            if(!color.present || width < 0) return;
            lv_draw_line_dsc_t dsc;
            lv_draw_line_dsc_init(&dsc);
            dsc.color = color.color;
            dsc.opa = color.opacity;
            dsc.width = std::max(1, width);
            dsc.p1.x = static_cast<lv_value_precise_t>(a.x);
            dsc.p1.y = static_cast<lv_value_precise_t>(a.y);
            dsc.p2.x = static_cast<lv_value_precise_t>(b.x);
            dsc.p2.y = static_cast<lv_value_precise_t>(b.y);
            lv_draw_line(&layer, &dsc);
        }
        static void flatten(std::vector<CanvasPoint> & points, CanvasPoint a, CanvasPoint b,
                            CanvasPoint c, CanvasPoint d, unsigned depth = 0) {
            // Bound the deviation from the chord to a quarter pixel, including loops.
            const double ux = 3*b.x - 2*a.x - d.x, uy = 3*b.y - 2*a.y - d.y;
            const double vx = 3*c.x - 2*d.x - a.x, vy = 3*c.y - 2*d.y - a.y;
            if(depth == 12 || std::max(ux*ux, vx*vx) + std::max(uy*uy, vy*vy) <= 1) {
                points.push_back(d);
                return;
            }
            CanvasPoint ab{(a.x+b.x)/2, (a.y+b.y)/2}, bc{(b.x+c.x)/2, (b.y+c.y)/2}, cd{(c.x+d.x)/2, (c.y+d.y)/2};
            CanvasPoint abc{(ab.x+bc.x)/2, (ab.y+bc.y)/2}, bcd{(bc.x+cd.x)/2, (bc.y+cd.y)/2};
            CanvasPoint middle{(abc.x+bcd.x)/2, (abc.y+bcd.y)/2};
            flatten(points, a, ab, abc, middle, depth+1);
            flatten(points, middle, bcd, cd, d, depth+1);
        }
        void render(lv_obj_t * object) const {
            lv_canvas_fill_bg(object, lv_color_black(), LV_OPA_TRANSP);
            for(const auto & command: commands) command(object);
        }

        public:
        const int width;
        const int height;
        const double pixel_density = 1.0;
        CanvasPainter(int width, int height, const lv_font_t * font = LV_FONT_DEFAULT)
            : font(font), width(width), height(height) {}
        CanvasPainter(const CanvasPainter &) = delete;
        CanvasPainter & operator=(const CanvasPainter &) = delete;

        void drawLine(double x1, double y1, double x2, double y2, CanvasColor color = {}, int width = 1) {
            if(!color.present) return;
            commands.push_back([=](lv_obj_t * object) {
                LVGL::detail::CanvasLayer layer(object);
                line(layer.layer, {x1, y1}, {x2, y2}, color, width);
            });
        }
        void drawPolyline(const std::vector<CanvasPoint> & points, CanvasColor color = {}, int width = 1) {
            if(!color.present || points.size() < 2) return;
            commands.push_back([=](lv_obj_t * object) {
                LVGL::detail::CanvasLayer layer(object);
                for(std::size_t i = 1; i < points.size(); ++i) line(layer.layer, points[i-1], points[i], color, width);
            });
        }
        void drawCubicBezier(CanvasPoint start, CanvasPoint control1, CanvasPoint control2, CanvasPoint end,
                             CanvasColor color = {}, int width = 1) {
            if(!color.present) return;
            std::vector<CanvasPoint> points{start};
            flatten(points, start, control1, control2, end);
            drawPolyline(points, color, width);
        }
        void drawPolygon(const std::vector<CanvasPoint> & points, CanvasColor fill = {},
                         CanvasColor stroke = {}, int width = 1) {
            if(points.empty()) return;
            const int canvasWidth = this->width, canvasHeight = this->height;
            commands.push_back([=](lv_obj_t * object) {
                LVGL::detail::CanvasLayer layer(object);
                if(fill.present && points.size() >= 3) {
                    // Even/odd scan conversion handles concave and self-intersecting polygons.
                    // Clip before generating spans so off-canvas coordinates cannot grow the work.
                    std::vector<double> crossings;
                    crossings.reserve(points.size());
                    for(int y = 0; y < canvasHeight; ++y) {
                        crossings.clear();
                        const double scan = y + 0.5;
                        for(std::size_t i = 0, j = points.size()-1; i < points.size(); j = i++) {
                            const auto a = points[j], b = points[i];
                            if((a.y <= scan && b.y > scan) || (b.y <= scan && a.y > scan))
                                crossings.push_back(a.x + (scan-a.y)*(b.x-a.x)/(b.y-a.y));
                        }
                        std::sort(crossings.begin(), crossings.end());
                        for(std::size_t i = 1; i < crossings.size(); i += 2) {
                            const double left = std::max(0.0, std::ceil(crossings[i-1]-0.5));
                            const double right = std::min(double(canvasWidth-1), std::ceil(crossings[i]-0.5)-1);
                            if(left > right) continue;
                            lv_draw_rect_dsc_t dsc;
                            lv_draw_rect_dsc_init(&dsc);
                            dsc.bg_color = fill.color;
                            dsc.bg_opa = fill.opacity;
                            lv_area_t area{int(left), y, int(right), y};
                            lv_draw_rect(&layer.layer, &dsc, &area);
                        }
                    }
                }
                if(stroke.present && points.size() > 1)
                    for(std::size_t i = 0, j = points.size()-1; i < points.size(); j = i++)
                        line(layer.layer, points[j], points[i], stroke, width);
            });
        }
        void drawRect(double x1, double y1, double x2, double y2, CanvasColor fill = {},
                      CanvasColor stroke = {}, int width = 1) {
            if(x1 > x2) std::swap(x1, x2);
            if(y1 > y2) std::swap(y1, y2);
            drawPolygon({{x1,y1}, {x2,y1}, {x2,y2}, {x1,y2}}, fill, stroke, width);
        }
        void drawEllipse(double x, double y, double rx, double ry, CanvasColor fill = {},
                         CanvasColor stroke = {}, int width = 1) {
            if(rx <= 0 || ry <= 0) return;
            const double pi = 3.14159265358979323846;
            const int count = int(std::min(4096.0, std::max(16.0, std::ceil(pi * std::sqrt(2*std::max(rx, ry))))));
            std::vector<CanvasPoint> points;
            points.reserve(count);
            for(int i = 0; i < count; ++i) {
                const double angle = 2*pi*i/count;
                points.push_back({x + rx*std::cos(angle), y + ry*std::sin(angle)});
            }
            drawPolygon(points, fill, stroke, width);
        }
        void drawText(double x, double y, const std::string & text, int w = -1, int h = -1,
                      int size = 12, CanvasColor color = {}, double rotate = 0, Anchor anchor = Anchor::LEFT_TOP) {
            if(w < 0) w = width;
            if(h < 0) h = height;
            if(w == 0 || h == 0 || size == 0 || text.empty()) return;
            // LVGL fonts are bitmaps: size is the line height in pixels. Negative keeps the native font.
            const auto nativeFont = font;
            const int scale = size < 0 ? LV_SCALE_NONE : std::max(1, size * LV_SCALE_NONE / int(font->line_height));
            commands.push_back([=](lv_obj_t * object) {
                lv_point_t bounds;
                lv_text_get_size(&bounds, text.c_str(), nativeFont, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_EXPAND);
                const int bw = std::min(bounds.x + 2, int(std::ceil(double(w)*LV_SCALE_NONE/scale)));
                const int bh = std::min(bounds.y, int(std::ceil(double(h)*LV_SCALE_NONE/scale)));
                if(bw <= 0 || bh <= 0) return;
                auto buffer = LVGL::detail::canvasBuffer(bw, bh);
                std::unique_ptr<lv_obj_t, decltype(&lv_obj_delete)> temporary(lv_canvas_create(object), lv_obj_delete);
                lv_obj_set_hidden(temporary.get(), true);
                lv_obj_set_ignore_layout(temporary.get(), true);
                lv_canvas_set_draw_buf(temporary.get(), buffer.get());
                lv_canvas_fill_bg(temporary.get(), lv_color_black(), LV_OPA_TRANSP);
                {
                    LVGL::detail::CanvasLayer layer(temporary.get());
                    lv_draw_label_dsc_t dsc;
                    lv_draw_label_dsc_init(&dsc);
                    dsc.font = nativeFont;
                    dsc.text = text.c_str();
                    dsc.flag = LV_TEXT_FLAG_EXPAND;
                    dsc.color = color.present ? color.color : lv_color_black();
                    dsc.opa = color.opacity;
                    lv_area_t area{0, 0, bw-1, bh-1};
                    lv_draw_label(&layer.layer, &dsc, &area);
                }
                // PUI measures the pivot using the native font before applying size.
                const double dx = bounds.x * (static_cast<int>(anchor)/3) / 2.0;
                const double dy = bounds.y * (static_cast<int>(anchor)%3) / 2.0;
                const double radians = rotate * 3.14159265358979323846 / 180;
                const int left = int(x+dx) + int(std::cos(radians)*int(-dx) - std::sin(radians)*int(-dy));
                const int top = int(y+dy) + int(std::sin(radians)*int(-dx) + std::cos(radians)*int(-dy));
                LVGL::detail::CanvasLayer layer(object);
                lv_draw_image_dsc_t image;
                lv_draw_image_dsc_init(&image);
                image.src = buffer.get();
                image.rotation = int(std::fmod(rotate, 360.0)*10);
                if(image.rotation < 0) image.rotation += 3600;
                image.scale_x = image.scale_y = scale;
                image.pivot = {0, 0};
                image.antialias = true;
                lv_area_t area{left, top, left+bw-1, top+bh-1};
                lv_draw_image(&layer.layer, &image, &area);
            });
        }
    };

    namespace LVGL {
        class Canvas: public Node {
            public:
            using Painter = std::function<bool(CanvasPainter &)>;
            Painter painter;
            private:
            struct Native: std::enable_shared_from_this<Native> {
                lv_obj_t * object = nullptr;
                Canvas * owner = nullptr;
                lv_timer_t * timer = nullptr;
                detail::CanvasBuffer buffer;
                Painter painter;
                std::uint64_t generation = 0;

                ~Native() { stop(); }
                void stop() {
                    painter = {};
                    ++generation;
                    if(timer) { lv_timer_delete(timer); timer = nullptr; }
                }
                void request() {
                    ++generation;
                    if(timer) { lv_timer_resume(timer); lv_timer_reset(timer); }
                }
                static void event(lv_event_t * event) {
                    auto holder = static_cast<std::shared_ptr<Native> *>(lv_event_get_user_data(event));
                    auto self = *holder;
                    if(lv_event_get_code(event) == LV_EVENT_DELETE) {
                        self->stop();
                        if(self->owner) self->owner->ui = nullptr;
                        self->owner = nullptr;
                        self->object = nullptr;
                        delete holder;
                    } else if(lv_event_get_code(event) == LV_EVENT_SIZE_CHANGED
                              || lv_event_get_code(event) == LV_EVENT_STYLE_CHANGED) self->request();
                }
                static void paint(lv_timer_t * timer) {
                    auto self = static_cast<Native *>(lv_timer_get_user_data(timer))->shared_from_this();
                    lv_timer_pause(timer);
                    try {
                        if(!self->object || !self->painter) return;
                        lv_obj_update_layout(self->object);
                        lv_timer_pause(timer);
                        const int width = lv_obj_get_width(self->object), height = lv_obj_get_height(self->object);
                        if(width <= 0 || height <= 0) return;
                        const auto version = self->generation;
                        auto invoke = self->painter;
                        CanvasPainter frame(width, height, lv_obj_get_style_text_font(self->object, LV_PART_MAIN));
                        const bool again = invoke(frame);
                        // A user callback may replace or remove this declaration/native object.
                        if(!self->object || version != self->generation) return;
                        if(!self->buffer || self->buffer->header.w != width || self->buffer->header.h != height) {
                            auto buffer = detail::canvasBuffer(width, height);
                            lv_canvas_set_draw_buf(self->object, buffer.get());
                            self->buffer = std::move(buffer);
                        }
                        frame.render(self->object);
                        if(again) self->request();
                    } catch(const std::exception & error) {
                        std::fprintf(stderr, "CPPUI LVGL Canvas paint failed: %s\n", error.what());
                    } catch(...) {
                        std::fprintf(stderr, "CPPUI LVGL Canvas paint failed: unknown exception\n");
                    }
                }
            };
            std::shared_ptr<Native> native;
            public:
            explicit Canvas(Painter painter): painter(std::move(painter)) { is_terminal = true; }
            void update(Node * prev) override {
                auto previous = static_cast<Canvas *>(prev);
                if(previous && previous->native && previous->native->object) native = std::move(previous->native);
                else {
                    native = std::make_shared<Native>();
                    std::unique_ptr<std::shared_ptr<Native>> holder(new std::shared_ptr<Native>(native));
                    native->object = lv_canvas_create(static_cast<lv_obj_t *>(parent->inner()));
                    lv_obj_set_size(native->object, 100, 100);
                    lv_obj_set_clickable(native->object, false);
                    lv_obj_set_scrollable(native->object, false);
                    lv_obj_add_event_cb(native->object, Native::event, LV_EVENT_ALL, holder.release());
                    native->timer = lv_timer_create(Native::paint, LV_DEF_REFR_PERIOD, native.get());
                }
                ui = native->object;
                native->owner = this;
                native->painter = painter;
                native->request();
            }
            void destroy(bool direct) override {
                if(native) {
                    native->stop();
                    native->owner = nullptr;
                    // With indirect deletion the native owner retains its buffer until LV_EVENT_DELETE.
                    if(direct && native->object) lv_obj_delete(native->object);
                    native.reset();
                }
                ui = nullptr;
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
    LVGL::Canvas & Canvas(Callback callback) {
        using Result = typename std::result_of<Callback &(CanvasPainter &)>::type;
        return *new LVGL::Canvas(LVGL::detail::canvasCallback(std::move(callback), std::is_void<Result>()));
    }
}
#endif

#include "cppui/lvgl/application.hpp"
#include "cppui/lvgl/window.hpp"
#include "cppui/lvgl/canvas.hpp"
#include <fstream>
#include <memory>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) {
        if(!value) throw std::runtime_error(message);
    }
    void step(int count = 1) {
        for(int i = 0; i < count; ++i) { lv_tick_inc(LV_DEF_REFR_PERIOD + 1); lv_timer_handler(); }
    }
    int captures = 0;
    struct Capture { Capture() { ++captures; } ~Capture() { --captures; } };
    class Root: public CPPUI::Application {
        public:
        using Application::Application;
        bool show = true, window = true, redrawDuringPaint = false, removeDuringPaint = false;
        bool deleteDuringPaint = false, fail = false, finished = false;
        int paints = 0, version = 1, seenVersion = 0, repeats = 0;
        int width = 0, height = 0;
        double density = 0;
        std::function<void(CPPUI::CanvasPainter &)> draw;
        void content() override {
            if(!window) return;
            for(auto win: CPPUI::Window()) if(show) {
                const int current = version;
                auto capture = std::make_shared<Capture>();
                CPPUI::Canvas([this, current, capture](CPPUI::CanvasPainter & canvas) {
                    ++paints;
                    seenVersion = current;
                    width = canvas.width; height = canvas.height; density = canvas.pixel_density;
                    if(redrawDuringPaint) { redrawDuringPaint = false; ++version; redraw(); }
                    if(removeDuringPaint) { removeDuringPaint = false; show = false; redraw(); }
                    if(deleteDuringPaint) { deleteDuringPaint = false; lv_obj_delete(native()); }
                    canvas.drawRect(10, 10, 30, 30, current == 1 ? 0xff0000 : 0x0000ff);
                    if(draw) draw(canvas);
                    if(fail) throw std::runtime_error("expected paint failure");
                    finished = true;
                    if(repeats > 0) { --repeats; return true; }
                    return false;
                });
            }
        }
        CPPUI::Node * node() { return children->front()->children->front(); }
        lv_obj_t * native() { return static_cast<lv_obj_t *>(node()->ui); }
    };
    bool pixel(lv_obj_t * object, int x, int y, unsigned rgb) {
        auto value = lv_canvas_get_px(object, x, y);
        return value.alpha == 255 && value.red == (rgb >> 16) && value.green == ((rgb >> 8) & 255)
            && value.blue == (rgb & 255);
    }
    int painted(lv_obj_t * object, int x1, int y1, int x2, int y2) {
        int result = 0;
        for(int y = y1; y < y2; ++y) for(int x = x1; x < x2; ++x)
            if(lv_canvas_get_px(object, x, y).alpha > 100) ++result;
        return result;
    }
    std::vector<std::uint32_t> pixels(lv_obj_t * object) {
        std::vector<std::uint32_t> result;
        const auto buffer = lv_canvas_get_draw_buf(object);
        for(int y = 0; y < buffer->header.h; ++y) for(int x = 0; x < buffer->header.w; ++x) {
            const auto p = lv_canvas_get_px(object, x, y);
            result.push_back((std::uint32_t(p.alpha)<<24) | (p.red<<16) | (p.green<<8) | p.blue);
        }
        return result;
    }
    void drawing(int argc, char ** argv) {
        Root root(argc, argv);
        root.draw = [](CPPUI::CanvasPainter & c) {
            c.drawRect(30, 30, 10, 10, "red");
            c.drawLine(40, 10, 80, 10, "#0f0", 2);
            c.drawPolyline({{40,20}, {60,20}, {60,30}}, 0xff00ff, 2);
            c.drawCubicBezier({40,40}, {50,40}, {70,40}, {80,40}, 0x00ffff, 2);
            c.drawCubicBezier({100,40}, {100,10}, {140,10}, {140,40}, 0xff00ff, 2);
            c.drawPolygon({{10,45}, {30,45}, {20,60}}, 0xffff00);
            c.drawEllipse(60,60,10,5,0x0000ff);
            c.drawPolygon({{90,50}, {120,50}, {120,60}, {100,60}, {100,80}, {90,80}}, 0x00ff00);
            c.drawPolygon({{130,50}, {150,70}, {130,70}, {150,50}}, 0xff0000);
            c.drawRect(-20,65,10,75,0x00ffff);
            c.drawRect(130,80,145,90, {}, "blue", 2);
            c.drawRect(140,95,150,105, "#80ff0000");
            c.drawLine(40,75,70,75); // Omitted colors do not paint or retain prior colors.
            c.drawRect(40,80,70,90);
            c.drawPolygon({}, 0xff0000);
            c.drawPolyline({}, 0xff0000);
            c.drawEllipse(10,10,0,0,0xffffff);
            c.drawText(0,100,std::string("Canvas"),-1,-1,12);
            c.drawText(90,100,"Rotate",60,40,12,"white",45,CPPUI::Anchor::CENTER);
        };
        root.redraw();
        auto object = root.native();
        lv_obj_set_size(object,160,150);
        step(2);
        require(root.width == 160 && root.height == 150 && root.density == 1, "canvas dimensions/density are wrong");
        require(pixel(object,20,20,0xff0000), "reversed rectangle corners did not normalize");
        require(pixel(object,60,10,0x00ff00), "line color/width/position is wrong");
        require(pixel(object,50,20,0xff00ff), "polyline did not render");
        require(pixel(object,60,40,0x00ffff), "straight cubic Bezier did not render");
        require(painted(object,110,16,130,23)>5, "curved cubic Bezier did not bend");
        require(pixel(object,20,50,0xffff00), "polygon fill did not render");
        require(pixel(object,60,60,0x0000ff), "ellipse fill did not render");
        require(pixel(object,95,75,0x00ff00) && !lv_canvas_get_px(object,110,75).alpha,
                "concave polygon filled its cutout");
        require(pixel(object,140,53,0xff0000) && !lv_canvas_get_px(object,131,60).alpha,
                "self-intersecting polygon lost even/odd fill");
        require(pixel(object,0,70,0x00ffff), "off-canvas polygon failed to clip");
        require(pixel(object,130,85,0x0000ff) && !lv_canvas_get_px(object,137,85).alpha,
                "outline-only rectangle filled its interior");
        require(lv_canvas_get_px(object,145,100).alpha == 128, "color alpha was lost");
        require(!lv_canvas_get_px(object,50,75).alpha && !lv_canvas_get_px(object,50,85).alpha,
                "omitted colors unexpectedly painted");
        require(painted(object,0,100,70,120)>25 && painted(object,80,100,140,140)>25,
                "plain or rotated text did not render");
        require(lv_obj_get_child_count(object) == 0, "temporary text canvases survived rendering");
        if(argc > 1) {
            std::ofstream image(argv[1], std::ios::binary);
            image << "P6\n160 150\n255\n";
            for(int y=0; y<150; ++y) for(int x=0; x<160; ++x) {
                auto p=lv_canvas_get_px(object,x,y);
                const unsigned char rgb[] = {static_cast<unsigned char>((p.red*p.alpha+85*(255-p.alpha))/255),
                    static_cast<unsigned char>((p.green*p.alpha+85*(255-p.alpha))/255),
                    static_cast<unsigned char>((p.blue*p.alpha+85*(255-p.alpha))/255)};
                image.write(reinterpret_cast<const char *>(rgb),3);
            }
        }
        const auto before = pixels(object);
        root.fail = true;
        root.version = 2;
        root.redraw();
        step(2);
        require(pixels(object) == before, "throwing callback replaced the last successful frame");
        root.fail = false;
        root.draw = [](CPPUI::CanvasPainter & c) {
            c.drawText(40,40,"Anchor",-1,-1,-1,0xffffff,90,CPPUI::Anchor::LEFT_TOP);
        };
        root.redraw(); step(2);
        const auto left = pixels(object);
        root.draw = [](CPPUI::CanvasPainter & c) {
            c.drawText(40,40,"Anchor",-1,-1,-1,0xffffff,90,CPPUI::Anchor::RIGHT_BOTTOM);
        };
        root.redraw(); step(2);
        require(left != pixels(object), "text anchor did not change its rotation pivot");
        root.draw = [](CPPUI::CanvasPainter & c) { c.drawText(40,40,"Size",-1,-1,8,0xffffff); };
        root.redraw(); step(2);
        const int small = painted(object,40,40,160,150);
        root.draw = [](CPPUI::CanvasPainter & c) { c.drawText(40,40,"Size",-1,-1,24,0xffffff); };
        root.redraw(); step(2);
        require(painted(object,40,40,160,150) > small*2, "text size did not scale the bitmap font");
        root.draw = [](CPPUI::CanvasPainter & c) { c.drawText(40,40,"clipped text",5,5,24,0xffffff); };
        root.redraw(); step(2);
        require(painted(object,46,40,160,150) == 0, "text ignored its width limit");
        // Rectangle fast path must preserve polygon coverage at fractional
        // boundaries, outside the buffer, and with translucent fill/stroke.
        root.draw = [](CPPUI::CanvasPainter & c) {
            c.drawRect(-2.5, 20.2, 50.5, 45.8, "#80ff0000", "blue", 2);
            c.drawRect(88.5, 81.5, 70.5, 60.5, "#80ffffff");
            c.drawRect(160.5, 5, 200, 20, "red");
        };
        root.redraw(); step(2); const auto rectangles = pixels(object);
        root.draw = [](CPPUI::CanvasPainter & c) {
            c.drawPolygon({{-2.5,20.2},{50.5,20.2},{50.5,45.8},{-2.5,45.8}}, "#80ff0000", "blue", 2);
            c.drawPolygon({{70.5,60.5},{88.5,60.5},{88.5,81.5},{70.5,81.5}}, "#80ffffff");
            c.drawPolygon({{160.5,5},{200,5},{200,20},{160.5,20}}, "red");
        };
        root.redraw(); step(2);
        require(rectangles == pixels(object), "rectangle fast path changed fill/stroke coverage");
        lv_obj_set_size(object,80,60); step(2);
        require(root.width == 80 && root.height == 60 && lv_canvas_get_draw_buf(object)->header.w == 80,
                "native resize failed to replace the backing buffer");
    }
    void lifetime(int argc, char ** argv) {
        Root root(argc, argv);
        root.redraw();
        auto object = root.native();
        root.version = 2;
        root.redraw();
        require(root.native() == object && captures == 1, "redraw replaced the native canvas or retained old captures");
        step(2);
        require(root.seenVersion == 2, "queued paint invoked a retired callback");
        const int settled = root.paints;
        step(4);
        require(root.paints == settled, "false failed to stop repeat painting");
        root.repeats = 2; root.redraw(); step(5);
        require(root.paints == settled+3, "true failed to request exactly the next frames");
        root.redrawDuringPaint = true; root.redraw(); step(3);
        require(root.seenVersion == 3 && captures == 1, "redraw during paint lost the new callback or retained captures");
        root.removeDuringPaint = true; root.finished = false; root.redraw(); step(3);
        require(root.finished && captures == 0 && lv_obj_get_child_count(lv_screen_active()) == 0,
                "self-removal invalidated callback or retained native resources");
        const int removed = root.paints;
        step(3);
        require(root.paints == removed, "removed canvas still painted");
        root.show = true; root.redraw(); step(2);
        root.deleteDuringPaint = true; root.redraw(); step(2);
        require(root.node()->ui == nullptr, "native deletion left a dangling observer");
        root.redraw(); step(2);
        require(root.native() && captures == 1, "canvas failed to recover after native deletion");
        root.redraw(); // Pending work must also be cancelled by indirect parent deletion.
        const int before = root.paints;
        root.window = false; root.redraw(); step(3);
        require(root.paints == before && captures == 0 && lv_obj_get_child_count(lv_screen_active()) == 0,
                "parent removal retained a callback, buffer or timer");
        root.window = true; root.redraw(); // Teardown while first paint is pending.
    }
    class VoidRoot: public CPPUI::Application {
        public:
        using Application::Application;
        int paints = 0;
        void content() override {
            for(auto window: CPPUI::Window()) CPPUI::Canvas([this](CPPUI::CanvasPainter & c) {
                ++paints;
                c.drawText(0,0,"void callback");
            });
        }
    };
}
int main(int argc, char ** argv) {
    lv_init();
    lv_display_create(200,160);
    int result = 0;
    try {
        drawing(argc,argv);
        require(captures == 0, "drawing teardown retained captures");
        lifetime(argc,argv);
        require(captures == 0, "application teardown retained captures");
        { VoidRoot root(argc,argv); root.redraw(); step(3); require(root.paints == 1, "void callback repeated or never ran"); }
        require(lv_obj_get_child_count(lv_screen_active()) == 0, "application teardown retained native canvases");
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    lv_deinit();
    return result;
}

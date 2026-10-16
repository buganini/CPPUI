#ifndef CPPUI_CANVAS_INPUT
#define CPPUI_CANVAS_INPUT
namespace CPPUI {
    // Local logical-pixel coordinates. Wheel angles and pixel distances retain
    // their native units; a positive vertical delta scrolls up.
    struct CanvasPointerEvent {
        double x = 0, y = 0;
        double angleX = 0, angleY = 0;
        double pixelX = 0, pixelY = 0;
    };
}
#endif

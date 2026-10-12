#include "backend.hpp"
#include "recipes.hpp"

namespace Cookbook {
    void canvas(Model & model) {
        const int angle = model.rotation;
        for(auto column: Presentation::VBox()) {
            for(auto row: Presentation::HBox()) {
                Presentation::Label("Rotate: " + std::to_string(angle));
                CPPUI::Button("+").click([&model]() { model.rotation += 15; });
            }
            Presentation::Canvas([angle](CPPUI::CanvasPainter & canvas) {
                canvas.drawText(50, 50, "___");
                canvas.drawText(50, 50, "PUI", -1, -1, 12, {}, angle, CPPUI::Anchor::CENTER);
                canvas.drawLine(20, 30, 70, 80, 0xFFFF00);
                canvas.drawPolyline({{10, 50}, {50, 10}, {70, 70}, {10, 50}}, 0xFF0000);
                canvas.drawCubicBezier({10, 80}, {25, 20}, {75, 80}, {90, 20}, 0x00FFFF, 2);
                canvas.drawRect(10, 10, 20, 20, 0xCCCCCC, 0xFF00FF, 2);
                return false;
            }).id("example-canvas");
            Presentation::Spacer();
        }
    }
}

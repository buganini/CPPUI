#include "cppui/frame_rate.hpp"
#include <stdexcept>
int main() {
    CPPUI::FrameRate rate;
    if(rate.tick(1).value() != "FPS: ..." || rate.tick(1.1) || rate.tick(1.4)
       || rate.tick(1.5).value() != "FPS: 6.0" || rate.tick(2.5).value() != "FPS: 1.0")
        throw std::runtime_error("completed-frame rate or publish interval is incorrect");
    rate.reset();
    if(rate.tick(10).value() != "FPS: ...") throw std::runtime_error("FPS restart retained old samples");
}

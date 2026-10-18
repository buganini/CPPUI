#ifndef CPPUI_FRAME_RATE
#define CPPUI_FRAME_RATE

#include "optional.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace CPPUI {
    // Tick only after a completed draw. Reporting does not schedule frames.
    class FrameRate {
        bool started = false;
        double start = 0;
        unsigned frames = 0;
        public:
        void reset() { started = false; frames = 0; }
        OptionalText tick(double now = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count()) {
            if(!started) { started = true; start = now; return std::string("FPS: ..."); }
            ++frames;
            const double elapsed = now - start;
            if(elapsed < .5) return {};
            std::ostringstream out;
            out << "FPS: " << std::fixed << std::setprecision(1) << frames / elapsed;
            start = now; frames = 0;
            return out.str();
        }
    };
}
#endif

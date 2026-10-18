#include "cppui/backend.hpp"
#include "recipes.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Cookbook {
    void timeline(Model &) {
        for(auto timer: CPPUI::TimelineView(1)) {
            const auto now = std::time(nullptr);
            std::ostringstream value;
            value << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
            CPPUI::Label(value.str());
        }
        CPPUI::Spacer();
    }
}

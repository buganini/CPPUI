#ifndef CPPUI_WX_PROGRESSBAR
#define CPPUI_WX_PROGRESSBAR
#include "base.hpp"
#include <cmath>
#include <limits>
namespace CPPUI { namespace Wx {
class ProgressBar: public Widget {
    static int hundredths(double value) {
        auto scaled = std::trunc(value * 100);
        if(!std::isfinite(scaled) || scaled > std::numeric_limits<int>::max() || scaled < std::numeric_limits<int>::min())
            throw std::out_of_range("ProgressBar value exceeds wx integer range");
        return static_cast<int>(scaled);
    }
public:
    int progress, maximum;
    ProgressBar(double progress, double maximum): progress(hundredths(progress)), maximum(hundredths(maximum)) { expandX = 1; }
    void updateWidget(Widget * prev) override {
        ui = prev ? prev->ui : new wxGauge(parentWindow(), wxID_ANY, 100);
        auto gauge = static_cast<wxGauge *>(ui);
        gauge->SetRange(std::max(1, maximum)); gauge->SetValue(std::max(0, std::min(gauge->GetRange(), progress)));
    }
};
}
inline Wx::ProgressBar & ProgressBar(double progress, double maximum = 1) { return *new Wx::ProgressBar(progress, maximum); }
}
#endif

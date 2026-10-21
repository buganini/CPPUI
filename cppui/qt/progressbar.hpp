#ifndef CPPUI_QT_PROGRESSBAR
#define CPPUI_QT_PROGRESSBAR

#include "base.hpp"
#include <QProgressBar>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace CPPUI {
    namespace Qt {
        class ProgressBar: public BaseWidget {
            static int hundredths(double value) {
                const double scaled = std::trunc(value * 100.0);
                if(!std::isfinite(scaled) || scaled < std::numeric_limits<int>::min()
                   || scaled > std::numeric_limits<int>::max()) {
                    throw std::out_of_range("ProgressBar value exceeds Qt's integer range");
                }
                return static_cast<int>(scaled);
            }

            public:
            int progress;
            int maximum;

            explicit ProgressBar(double progress, double maximum = 1.0)
                : progress(hundredths(progress)), maximum(hundredths(maximum)) { expandX=1; }

            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new Widget<QProgressBar>();
                auto bar = static_cast<QProgressBar *>(ui);
                bar->setMaximum(maximum);
                bar->setValue(progress);
            }
        };
    }
    inline Qt::ProgressBar & ProgressBar(double progress, double maximum = 1.0) {
        return *(new Qt::ProgressBar(progress, maximum));
    }
}
#endif

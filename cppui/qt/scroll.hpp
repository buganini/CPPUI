#ifndef CPPUI_QT_SCROLL
#define CPPUI_QT_SCROLL

#include "base.hpp"
#include "../optional.hpp"
#include "../scroll.hpp"
#include <QScrollArea>
#include <QScrollBar>
#include <QEvent>
#include <cmath>

namespace CPPUI { namespace Qt {
    struct ScrollAxis {
        bool end = false;
        double offset = 0;
        void set(double value) {
            if(!std::isfinite(value)) throw std::invalid_argument("Scroll offset must be finite");
            end = std::signbit(value); offset = std::abs(value);
        }
        void capture(QScrollBar * bar) { offset = end ? bar->maximum() - bar->value() : bar->value(); }
    };
    class ScrollWidget: public Widget<QScrollArea> {
        bool horizontalDisabled, verticalDisabled;
        static ::Qt::ScrollBarPolicy policy(const Optional<bool> & value) {
            return !value ? ::Qt::ScrollBarAsNeeded : *value ? ::Qt::ScrollBarAlwaysOn : ::Qt::ScrollBarAlwaysOff;
        }
        void observe(QScrollBar * bar, ScrollAxis & axis) {
            QObject::connect(bar, &QScrollBar::valueChanged, this, [bar, &axis](int value) {
                if(value < 10 && value > bar->maximum() - 10) return;
                if(value < 10) axis.end = false;
                else if(value > bar->maximum() - 10) axis.end = true;
            });
            QObject::connect(bar, &QScrollBar::rangeChanged, this, [bar, &axis](int minimum, int maximum) {
                const double value = axis.end ? maximum - axis.offset : axis.offset;
                bar->setValue(static_cast<int>(std::max(double(minimum), std::min(double(maximum), value))));
            });
        }
        protected:
        bool eventFilter(QObject * object, QEvent * event) override {
            if(object == widget() && event->type() == QEvent::Resize) fitDisabledAxes();
            return QScrollArea::eventFilter(object, event);
        }
        public:
        ScrollAxis x, y;
        ScrollWidget(Optional<bool> vertical, Optional<bool> horizontal, ScrollAxis x, ScrollAxis y)
            : horizontalDisabled(horizontal && !*horizontal), verticalDisabled(vertical && !*vertical), x(x), y(y) {
            setWidgetResizable(true);
            setVerticalScrollBarPolicy(policy(vertical)); setHorizontalScrollBarPolicy(policy(horizontal));
            observe(horizontalScrollBar(), this->x); observe(verticalScrollBar(), this->y);
        }
        void configureSizing(Optional<bool> vertical, Optional<bool> horizontal) {
            horizontalDisabled = horizontal && !*horizontal;
            verticalDisabled = vertical && !*vertical;
        }
        void fitDisabledAxes() {
            if(auto child = widget()) {
                // QWidget uses -1 for an unspecified hint, not a minimum size.
                const auto hint=child->sizeHint();
                if(horizontalDisabled) setMinimumWidth(std::max(0,hint.width()));
                if(verticalDisabled) setMinimumHeight(std::max(0,hint.height()));
            }
        }
    };
    class Scroll: public BaseWidget {
        Optional<bool> vertical, horizontal;
        ScrollAxis x, y;
        public:
        static constexpr double END = CPPUI::ScrollEnd;
        Scroll(Optional<bool> vertical = {}, Optional<bool> horizontal = false): vertical(vertical), horizontal(horizontal) {
            expandX=horizontal && !*horizontal ? 1 : 4;expandY=vertical && !*vertical ? 1 : 4;
        }
        Scroll & scrollX(double position = 0) {
            (ui ? static_cast<ScrollWidget *>(ui)->x : x).set(position); return *this;
        }
        Scroll & scrollY(double position = 0) {
            (ui ? static_cast<ScrollWidget *>(ui)->y : y).set(position); return *this;
        }
        void update(BaseWidget * prev) override {
            ui = prev && prev->ui ? prev->ui : new ScrollWidget(vertical, horizontal, x, y);
            static_cast<ScrollWidget *>(ui)->configureSizing(vertical, horizontal);
        }
        void preSync() override {
            if(!ui) return;
            auto scroll = static_cast<ScrollWidget *>(ui);
            scroll->x.capture(scroll->horizontalScrollBar()); scroll->y.capture(scroll->verticalScrollBar());
        }
        void postSync() override {
            if(auto scroll = static_cast<ScrollWidget *>(ui)) {
                if(scroll->widget()) scroll->widget()->installEventFilter(scroll);
                scroll->fitDisabledAxes();
            }
        }
        void addChild(int index, Node * child) override {
            if(child->is_outoforder) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(!widget || !widget->ui) return;
            auto scroll = static_cast<ScrollWidget *>(ui);
            if(index == 0) {
                // setWidget deletes its old content. Keep a still-mounted
                // declaration alive if an insertion makes it an ignored sibling.
                if(auto previous = scroll->takeWidget()) {
                    previous->removeEventFilter(scroll);
                    previous->setParent(scroll->viewport()); previous->hide();
                }
                widget->ui->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
                scroll->setWidget(widget->ui);
                widget->ui->installEventFilter(scroll);
            } else { widget->ui->setParent(scroll->viewport()); widget->ui->hide(); }
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder || !ui) return;
            auto widget = dynamic_cast<BaseWidget *>(child);
            if(!widget || !widget->ui) return;
            auto scroll = static_cast<ScrollWidget *>(ui);
            widget->ui->removeEventFilter(scroll);
            if(scroll->widget() == widget->ui) scroll->takeWidget();
            widget->ui->setParent(nullptr);
        }
    };
}}
namespace CPPUI {
    inline Qt::Scroll & Scroll(Optional<bool> vertical = {}, Optional<bool> horizontal = false) { return *new Qt::Scroll(vertical, horizontal); }
}
#endif

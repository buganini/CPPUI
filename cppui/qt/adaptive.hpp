#ifndef CPPUI_QT_ADAPTIVE
#define CPPUI_QT_ADAPTIVE
#include "../view.hpp"
#include <QMainWindow>
#include <QPointer>
#include <QLayout>
#include <QTimer>
#include <QEvent>

namespace CPPUI { namespace Qt {
    class SizeObserver: public QObject, public detail::SizeObserver {
        QPointer<QWidget> widget;
        std::weak_ptr<detail::SizeState> state;
        bool pending=false;
    public:
        SizeObserver(QWidget * target,std::shared_ptr<detail::SizeState> state):widget(target),state(state) {
            widget->installEventFilter(this);
        }
        ~SizeObserver() override { if(widget) widget->removeEventFilter(this); }
        void refresh() override {
            if(pending || !widget) return;
            pending=true;
            QTimer::singleShot(0,this,[this]() {
                pending=false;
                if(!widget) return;
                QWidget * content=widget;
                if(auto window=qobject_cast<QMainWindow *>(content))
                    if(window->centralWidget()) content=window->centralWidget();
                auto rect=content->contentsRect();
                if(auto layout=content->layout()) rect=rect.marginsRemoved(layout->contentsMargins());
                auto current=state.lock();
                if(current) current->publish(rect.width(),rect.height());
            });
        }
        bool eventFilter(QObject *,QEvent * event) override {
            switch(event->type()) {
                case QEvent::Resize: case QEvent::Show: case QEvent::LayoutRequest: case QEvent::ContentsRectChange:
                    refresh();break;
                default:break;
            }
            return false;
        }
    };
}}
#endif

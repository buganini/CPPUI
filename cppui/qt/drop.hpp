#ifndef CPPUI_QT_DROP
#define CPPUI_QT_DROP
#include "../drop.hpp"
#include <QDragEnterEvent>
#include <QAbstractScrollArea>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QPointer>
#include <QWidget>
#include <iostream>

namespace CPPUI { namespace Qt {
    inline DropEvent dropData(const QDropEvent & event) {
        DropEvent result;
        const auto mime=event.mimeData();
        for(const auto & url:mime->urls()) {
            if(url.isLocalFile()) {
                const auto path=url.toLocalFile();
                result.files.emplace_back(QFileInfo(path).fileName().toStdString(),path.toStdString());
            } else result.urls.push_back(url.toString().toStdString());
        }
        if(mime->hasText()) result.text=mime->text().toStdString();
        result.x=event.position().x(); result.y=event.position().y();
        result.action=event.proposedAction()==::Qt::MoveAction ? "move" : event.proposedAction()==::Qt::LinkAction ? "link" : "copy";
        return result;
    }
    class DropFilter:public QObject {
        QPointer<QWidget> widget;
        public:
        ::CPPUI::detail::DropHandlers handlers;
        explicit DropFilter(QWidget * widget):QObject(widget),widget(widget) {
            widget->installEventFilter(this);
            if(auto scroll=qobject_cast<QAbstractScrollArea *>(widget)) scroll->viewport()->installEventFilter(this);
        }
        void enable(bool value) {
            if(!widget) return;
            widget->setAcceptDrops(value);
            if(auto scroll=qobject_cast<QAbstractScrollArea *>(widget.data())) scroll->viewport()->setAcceptDrops(value);
        }
        bool eventFilter(QObject *,QEvent * event) override {
            const bool entering=event->type()==QEvent::DragEnter;
            if(!entering && event->type()!=QEvent::Drop) return false;
            auto native=static_cast<QDropEvent *>(event);
            auto data=dropData(*native); auto callbacks=handlers; auto alive=widget; QPointer<DropFilter> self(this);
            native->ignore();
            try {
                if(entering) {
                    if(callbacks.entered) callbacks.entered(data);
                    if(self && self->handlers.dropped && alive && alive->isEnabled()) native->acceptProposedAction();
                } else {
                    callbacks.dispatch(data,alive && alive->isEnabled());
                    if(data.accepted) native->acceptProposedAction();
                }
            } catch(const std::exception & error) { std::cerr<<"CPPUI drop: "<<error.what()<<'\n'; }
            catch(...) { std::cerr<<"CPPUI drop handler failed\n"; }
            return true;
        }
    };
}}
#endif

#ifndef CPPUI_QT_TEXT
#define CPPUI_QT_TEXT

#include "base.hpp"
#include "url.hpp"
#include <QLabel>
#include <string>

namespace CPPUI {
    namespace Qt {
        class Text: public BaseWidget {
            protected:
            ::Qt::TextFormat format = ::Qt::PlainText;

            public:
            std::string text;
            bool selectable;

            explicit Text(const std::string & text, bool selectable = false)
                : text(text), selectable(selectable) {}

            void update(BaseWidget * prev) override {
                ui = prev && prev->ui ? prev->ui : new Widget<QLabel>();
                auto label = static_cast<QLabel *>(ui);
                label->setTextFormat(format);
                label->setText(QString::fromStdString(text));
                label->setWordWrap(true);
                label->setAlignment(::Qt::AlignTop);
                label->setTextInteractionFlags(selectable ? ::Qt::TextSelectableByMouse : ::Qt::NoTextInteraction);
            }
        };
        class Html: public Text {
            bool allowURLs=true;
            public:
            Html & openURL(bool value=true) {allowURLs=value;return *this;}
            void update(BaseWidget * prev) override {
                Text::update(prev);auto label=static_cast<QLabel *>(ui);
                label->setOpenExternalLinks(false);
                if(allowURLs) label->setTextInteractionFlags(label->textInteractionFlags() | ::Qt::LinksAccessibleByMouse | ::Qt::LinksAccessibleByKeyboard);
                const bool enabled=allowURLs;
                connectSignal(QObject::connect(label,&QLabel::linkActivated,label,[enabled](const QString & url){
                    if(enabled) try {CPPUI::OpenURL(url.toStdString());}catch(const std::exception & e) {std::cerr<<"CPPUI Html link: "<<e.what()<<'\n';}
                }));
            }
            explicit Html(const std::string & text, bool selectable = false): Text(text, selectable) {
                format = ::Qt::RichText; expandX=expandY=4;
            }
        };
        class MarkDown: public Text {
            public:
            explicit MarkDown(const std::string & text, bool selectable = false): Text(text, selectable) {
                format = ::Qt::MarkdownText;
            }
        };
    }
    inline Qt::Text & Text(const std::string & text, bool selectable = false) {
        return *(new Qt::Text(text, selectable));
    }
    inline Qt::Html & Html(const std::string & text, bool selectable = false) {
        return *(new Qt::Html(text, selectable));
    }
    inline Qt::MarkDown & MarkDown(const std::string & text, bool selectable = false) {
        return *(new Qt::MarkDown(text, selectable));
    }
}
#endif

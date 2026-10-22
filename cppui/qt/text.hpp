#ifndef CPPUI_QT_TEXT
#define CPPUI_QT_TEXT

#include "base.hpp"
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
            public:
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

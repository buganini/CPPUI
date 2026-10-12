#include "cppui/qt/button.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/progressbar.hpp"
#include "cppui/qt/text.hpp"
#include <limits>
#include <QApplication>
#include <QEvent>
#include <stdexcept>

namespace {
    void require(bool condition, const char * message) {
        if(!condition) throw std::runtime_error(message);
    }

    class Root: public CPPUI::Qt::QtView {
        public:
        bool selectable = true;
        double progress = 0.125;
        double maximum = 0.25;
        std::string caption = std::string(100, 'x');

        ~Root() {
            for(auto child: *children) CPPUI::recur_delete(child, true);
            children->clear();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        void content() override {
            // Both arguments die before native reconciliation starts.
            CPPUI::Button(std::string(caption)).id(std::string(100, 'b'));
            CPPUI::Label("No <b>HTML</b>", selectable);
            CPPUI::ProgressBar(progress, maximum);
            CPPUI::Text(caption, selectable);
            CPPUI::Html("<i>HTML</i>");
            CPPUI::MarkDown("# Heading");
        }
    };
}

int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        Root root;
        root.sync();
        auto label = static_cast<QLabel *>(root.children->at(1)->outer());
        require(label->textFormat() == Qt::PlainText, "Label interpreted markup as HTML");
        auto button = static_cast<QPushButton *>(root.children->front()->outer());
        require(button->text().toStdString() == root.caption, "temporary button text was lost");
        require(label->textInteractionFlags() == Qt::TextSelectableByMouse, "Label selection was not enabled");
        auto text = static_cast<QLabel *>(root.children->at(3)->outer());
        auto html = static_cast<QLabel *>(root.children->at(4)->outer());
        auto markdown = static_cast<QLabel *>(root.children->at(5)->outer());
        require(text->textFormat() == Qt::PlainText && text->wordWrap()
                && (text->alignment() & Qt::AlignTop), "plain Text presentation differs from PUI");
        require(html->textFormat() == Qt::RichText && markdown->textFormat() == Qt::MarkdownText,
                "rich text format was not applied");
        require(text->textInteractionFlags() == Qt::TextSelectableByMouse, "Text selection was not enabled");
        root.selectable = false;
        root.caption.assign(120, 'y');
        root.sync();
        require(root.children->front()->outer() == button, "temporary ID changed native identity");
        require(button->text().toStdString() == root.caption, "retained button text was not updated");
        require(root.children->at(3)->outer() == text && text->text().toStdString() == root.caption,
                "Text did not reuse its native widget with updated content");
        require(label->textInteractionFlags() == Qt::NoTextInteraction
                && text->textInteractionFlags() == Qt::NoTextInteraction,
                "redraw did not clear selection flags");
        auto bar = static_cast<QProgressBar *>(root.children->at(2)->outer());
        require(bar->maximum() == 25 && bar->value() == 12, "fractional hundredths conversion differs from PUI");
        root.maximum = 2.5;
        root.progress = 1.25;
        root.sync();
        require(root.children->at(2)->outer() == bar, "progress change replaced the native widget");
        require(bar->maximum() == 250 && bar->value() == 125, "progress properties were not updated");
        root.progress = std::numeric_limits<double>::infinity();
        bool rejected = false;
        try { root.sync(); } catch(const std::out_of_range &) { rejected = true; }
        require(rejected && root.children->at(2)->outer() == bar, "invalid progress damaged the mounted tree");
        root.progress = 0;
        root.maximum = 0;
        root.sync();
        require(bar->minimum() == 0 && bar->maximum() == 0, "busy range did not match Qt semantics");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

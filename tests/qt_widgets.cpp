#include "cppui/qt/button.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include "cppui/qt/progressbar.hpp"
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
            CPPUI::Label("No <b>HTML</b>");
            CPPUI::ProgressBar(progress, maximum);
        }
    };
}

int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        Root root;
        root.redraw();
        auto label = static_cast<QLabel *>(root.children->at(1)->outer());
        require(label->textFormat() == Qt::PlainText, "Label interpreted markup as HTML");
        auto button = static_cast<QPushButton *>(root.children->front()->outer());
        require(button->text().toStdString() == root.caption, "temporary button text was lost");
        root.caption.assign(120, 'y');
        root.redraw();
        require(root.children->front()->outer() == button, "temporary ID changed native identity");
        require(button->text().toStdString() == root.caption, "retained button text was not updated");
        auto bar = static_cast<QProgressBar *>(root.children->at(2)->outer());
        require(bar->maximum() == 25 && bar->value() == 12, "fractional hundredths conversion differs from PUI");
        root.maximum = 2.5;
        root.progress = 1.25;
        root.redraw();
        require(root.children->at(2)->outer() == bar, "progress change replaced the native widget");
        require(bar->maximum() == 250 && bar->value() == 125, "progress properties were not updated");
        root.progress = std::numeric_limits<double>::infinity();
        bool rejected = false;
        try { root.redraw(); } catch(const std::out_of_range &) { rejected = true; }
        require(rejected && root.children->at(2)->outer() == bar, "invalid progress damaged the mounted tree");
        root.progress = 0;
        root.maximum = 0;
        root.redraw();
        require(bar->minimum() == 0 && bar->maximum() == 0, "busy range did not match Qt semantics");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

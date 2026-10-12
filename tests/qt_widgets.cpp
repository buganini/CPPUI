#include "cppui/qt/button.hpp"
#include "cppui/qt/label.hpp"
#include "cppui/qt/layout.hpp"
#include <QApplication>
#include <QEvent>
#include <stdexcept>

namespace {
    void require(bool condition, const char * message) {
        if(!condition) throw std::runtime_error(message);
    }

    class Root: public CPPUI::Qt::QtView {
        public:
        std::string caption = std::string(100, 'x');

        ~Root() {
            for(auto child: *children) CPPUI::recur_delete(child, true);
            children->clear();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        void content() override {
            // Both arguments die before native reconciliation starts.
            CPPUI::Button(std::string(caption)).id(std::string(100, 'b'));
        }
    };
}

int main(int argc, char ** argv) {
    QApplication app(argc, argv);
    try {
        Root root;
        root.redraw();
        auto button = static_cast<QPushButton *>(root.children->front()->outer());
        require(button->text().toStdString() == root.caption, "temporary button text was lost");
        root.caption.assign(120, 'y');
        root.redraw();
        require(root.children->front()->outer() == button, "temporary ID changed native identity");
        require(button->text().toStdString() == root.caption, "retained button text was not updated");
    } catch(const std::exception & error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}

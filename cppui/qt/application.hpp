#ifndef CPPUI_QT_APPLICATION
#define CPPUI_QT_APPLICATION

#include "base.hpp"
#include <QApplication>
#include <QEvent>
#include <memory>
#include <stdexcept>

namespace CPPUI {
    class Application: public Qt::QtView {
        int argc;
        char ** argv;
        std::unique_ptr<QApplication> ownedApplication;
        QApplication * application = nullptr;
        std::unique_ptr<QObject> quitContext{new QObject()};

        public:
        Application(int argc, char * argv[]): argc(argc), argv(argv) {}

        ~Application() override {
            // QObject destruction removes queued calls before native teardown.
            quitContext.reset();
            // Flush only this application's native roots while Qt is still live.
            std::vector<QWidget *> roots;
            collectRoots(children, roots);
            clearChildren();
            for(auto root: roots) QCoreApplication::sendPostedEvents(root, QEvent::DeferredDelete);
        }

        void update(Node * prev) override {
            if(!application) {
                application = qobject_cast<QApplication *>(QCoreApplication::instance());
                if(!application) {
                    if(QCoreApplication::instance()) {
                        throw std::logic_error("CPPUI requires QApplication, not QCoreApplication");
                    }
                    ownedApplication.reset(new QApplication(argc, argv));
                    application = ownedApplication.get();
                }
            }
            QtView::update(prev);
        }

        void addChild(int, Node * child) override {
            if(auto widget = static_cast<QWidget *>(child->outer())) widget->show();
        }
        void removeChild(int, Node * child) override {
            if(auto widget = static_cast<QWidget *>(child->outer())) widget->close();
        }
        void start() override { application->exec(); }

        void quit() {
            if(destroyed || retired_by) return;
            QMetaObject::invokeMethod(quitContext.get(), [this]() {
                if(!destroyed && !retired_by && application) application->quit();
            }, ::Qt::QueuedConnection);
        }

        private:
        static void collectRoots(const std::vector<Node *> * nodes, std::vector<QWidget *> & roots) {
            for(auto node: *nodes) {
                if(node->is_virtual) collectRoots(node->children, roots);
                else if(auto widget = static_cast<QWidget *>(node->outer())) roots.push_back(widget);
            }
        }
    };
}
#endif

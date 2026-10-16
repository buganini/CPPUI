#ifndef CPPUI_QT_APPLICATION
#define CPPUI_QT_APPLICATION

#include "base.hpp"
#include "../application_title.hpp"
#include <QApplication>
#include <QEvent>
#include <QIcon>
#include <memory>
#include <stdexcept>

namespace CPPUI {
    class Application: public Qt::QtView, public ApplicationTitle {
        int argc;
        char ** argv;
        std::string iconPath;
        std::string caption;
        std::unique_ptr<QApplication> ownedApplication;
        QApplication * application = nullptr;
        std::unique_ptr<QObject> quitContext{new QObject()};
        QtOptions qtParams;
        QStyle * appliedStyle = nullptr;
        QString appliedSheet;
        bool sheetApplied = false;

        public:
        Application(int argc, char * argv[], const std::string & icon = {})
            : argc(argc), argv(argv), iconPath(icon) {}
        Application & title(const std::string & text) { caption = text; return *this; }
        const std::string & applicationTitle() const override { return caption; }

        Node & qt(const QtOptions & options) override {
            if(options.HorizontalPolicy || options.VerticalPolicy || options.SizeConstraint)
                throw std::invalid_argument("Application qt() accepts Style and StyleSheet, not widget layout options");
            qtParams.merge(options); return *this;
        }

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
                // PUI applies the icon only at initialization. An omitted icon
                // leaves a borrowed application's existing default unchanged.
                if(!iconPath.empty()) application->setWindowIcon(QIcon(QString::fromStdString(iconPath)));
            }
            if(!caption.empty()) application->setApplicationDisplayName(QString::fromStdString(caption));
            QtView::update(prev);
            if(qtParams.Style && *qtParams.Style && *qtParams.Style != appliedStyle) {
                appliedStyle = *qtParams.Style;
                application->setStyle(appliedStyle);
            }
            const auto sheet = Qt::styleSheet(*this, qtParams);
            if(sheet != appliedSheet || (qtParams.StyleSheet && !sheetApplied)) {
                application->setStyleSheet(sheet); appliedSheet = sheet; sheetApplied = true;
            }
        }

        void addChild(int, Node * child) override {
            if(child->is_outoforder) return;
            if(auto widget = static_cast<QWidget *>(child->outer())) widget->show();
        }
        void removeChild(int, Node * child) override {
            if(child->is_outoforder) return;
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

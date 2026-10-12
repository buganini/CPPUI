#include "cppui/qt/dialog.hpp"
#include <QAbstractButton>
#include <QApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QTimer>
#include <algorithm>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
    void require(bool value, const char * message) { if(!value) throw std::runtime_error(message); }
    class Driver {
        QTimer poll, watchdog;
        std::exception_ptr failure;
        bool invoked = false;
        public:
        explicit Driver(std::function<void(QDialog *)> action) {
            QObject::connect(&poll, &QTimer::timeout, [& , action]() {
                auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                if(!dialog) return;
                poll.stop();
                invoked = true;
                try { action(dialog); }
                catch(...) { failure = std::current_exception(); dialog->reject(); }
            });
            QObject::connect(&watchdog, &QTimer::timeout, [&]() {
                failure = std::make_exception_ptr(std::runtime_error("dialog interaction timed out"));
                if(auto dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) dialog->reject();
                else QCoreApplication::exit(1);
            });
            poll.start(1);
            watchdog.setSingleShot(true);
            watchdog.start(5000);
        }
        void check() {
            poll.stop(); watchdog.stop();
            if(failure) std::rethrow_exception(failure);
            require(invoked, "dialog returned without interaction");
        }
    };
    void promptAndMessages() {
        for(bool accepted: {true, false}) {
            Driver driver([accepted](QDialog * native) {
                auto dialog = qobject_cast<QInputDialog *>(native);
                require(dialog && dialog->windowTitle() == "Title" && dialog->labelText() == "Question"
                    && dialog->textValue() == "Default", "prompt lost title/label/default");
                dialog->setTextValue(accepted ? "" : "discard me");
                accepted ? dialog->accept() : dialog->reject();
            });
            const auto result = CPPUI::Prompt("Question", "Title", "Default");
            driver.check();
            require(accepted ? (result && result->empty()) : !result, "prompt confused empty acceptance with cancellation");
        }
        {
            Driver driver([](QDialog * native) {
                auto dialog = qobject_cast<QInputDialog *>(native);
                require(dialog, "prompt did not use an input dialog");
                dialog->setTextValue(QString::fromUtf8("hello 世界"));
                dialog->accept();
            });
            const auto result = CPPUI::Prompt();
            driver.check();
            require(result && *result == "hello 世界", "prompt text did not round-trip UTF-8");
        }
        for(auto button: {QMessageBox::Yes, QMessageBox::No}) {
            Driver driver([button](QDialog * native) {
                auto dialog = qobject_cast<QMessageBox *>(native);
                require(dialog && dialog->text() == "Confirm message" && dialog->windowTitle() == "Confirm title"
                    && dialog->icon() == QMessageBox::Question, "confirm lost content/icon");
                dialog->button(button)->click();
            });
            const bool result = CPPUI::Confirm("Confirm message", "Confirm title");
            driver.check();
            require(result == (button == QMessageBox::Yes), "confirm result is wrong");
        }
        for(auto icon: {QMessageBox::Information, QMessageBox::Warning, QMessageBox::Critical}) {
            Driver driver([icon](QDialog * native) {
                auto dialog = qobject_cast<QMessageBox *>(native);
                require(dialog && dialog->icon() == icon && dialog->text() == "Message"
                    && dialog->windowTitle() == "Title" && dialog->standardButtons() == QMessageBox::Ok,
                    "message dialog lost icon/text/title/buttons");
                dialog->button(QMessageBox::Ok)->click();
            });
            if(icon == QMessageBox::Information) CPPUI::Information("Message", "Title");
            if(icon == QMessageBox::Warning) CPPUI::Warning("Message", "Title");
            if(icon == QMessageBox::Critical) CPPUI::Critical("Message", "Title");
            driver.check();
        }
    }
    void files() {
        QTemporaryDir directory;
        require(directory.isValid(), "cannot create dialog test directory");
        const auto first = directory.filePath("one.txt");
        const auto second = directory.filePath("two.txt");
        for(const auto & path: {first, second}) {
            QFile file(path);
            require(file.open(QIODevice::WriteOnly), "cannot create dialog fixture");
        }
        {
            Driver driver([&](QDialog * native) {
                auto dialog = qobject_cast<QFileDialog *>(native);
                require(dialog && dialog->windowTitle() == "Choose" && dialog->fileMode() == QFileDialog::ExistingFile
                    && dialog->directory().absolutePath() == directory.path()
                    && dialog->nameFilters() == QStringList({"Text (*.txt)", "All (*)"}), "file arguments/filters were lost");
                dialog->selectFile(first);
                native->accept();
            });
            const auto result = CPPUI::OpenFile("Choose", directory.path().toStdString(), "Text (*.txt)|All (*)");
            driver.check();
            require(result && *result == first.toStdString(), "open-file result was lost");
        }
        {
            Driver driver([&](QDialog * native) {
                auto dialog = qobject_cast<QFileDialog *>(native);
                require(dialog && dialog->fileMode() == QFileDialog::ExistingFiles, "multi-file mode is wrong");
                auto edit = dialog->findChild<QLineEdit *>("fileNameEdit");
                require(edit, "file selection editor unavailable");
                edit->setText("\"one.txt\" \"two.txt\"");
                native->accept();
            });
            auto result = CPPUI::OpenFiles("Open Files", directory.path().toStdString());
            driver.check();
            require(result && result->size() == 2, "multi-file result was lost");
            std::sort(result->begin(), result->end());
            require((*result)[0] == first.toStdString() && (*result)[1] == second.toStdString(), "wrong selected files");
        }
        {
            Driver driver([&](QDialog * native) {
                auto dialog = qobject_cast<QFileDialog *>(native);
                require(dialog && dialog->fileMode() == QFileDialog::Directory, "directory mode is wrong");
                dialog->selectFile(directory.path());
                native->accept();
            });
            const auto result = CPPUI::OpenDirectory("Directory", directory.path().toStdString());
            driver.check();
            require(result && *result == directory.path().toStdString(), "directory result was lost");
        }
        const auto saved = directory.filePath("new.txt");
        for(bool explicitPath: {false, true}) {
            Driver driver([&](QDialog * native) {
                auto dialog = qobject_cast<QFileDialog *>(native);
                require(dialog && dialog->acceptMode() == QFileDialog::AcceptSave && dialog->windowTitle() == "Save"
                    && dialog->selectedFiles() == QStringList{saved}, "save default/explicit path precedence is wrong");
                native->accept();
            });
            const auto result = CPPUI::SaveFile(CPPUI::OptionalText(explicitPath ? "ignored" : saved.toStdString()),
                "Save", explicitPath ? saved.toStdString() : "", "Text (*.txt)");
            driver.check();
            require(result && *result == saved.toStdString(), "save result was lost");
        }
        for(int kind = 0; kind < 4; ++kind) {
            Driver driver([](QDialog * dialog) { dialog->reject(); });
            bool present = false;
            const auto path = directory.path().toStdString();
            if(kind == 0) present = bool(CPPUI::OpenDirectory("Cancel", path));
            if(kind == 1) present = bool(CPPUI::OpenFile("Cancel", path));
            if(kind == 2) present = bool(CPPUI::OpenFiles("Cancel", path));
            if(kind == 3) present = bool(CPPUI::SaveFile(CPPUI::OptionalText(), "Cancel", path));
            driver.check();
            require(!present, "cancelled file dialog returned a value");
        }
    }
}
int main(int argc, char ** argv) {
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    try {
        promptAndMessages();
        files();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        for(auto widget: QApplication::topLevelWidgets()) {
            require(!qobject_cast<QDialog *>(widget), "dialog wrapper retained a dialog after returning");
        }
    } catch(const std::exception & error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

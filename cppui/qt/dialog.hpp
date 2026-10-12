#ifndef CPPUI_QT_DIALOG
#define CPPUI_QT_DIALOG

#include "../optional.hpp"
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <string>
#include <vector>

namespace CPPUI {
    namespace Qt { namespace detail {
        inline QString fileFilters(const std::string & types) {
            auto filters = QString::fromStdString(types);
            return filters.replace("|", ";;");
        }
        inline OptionalText fileResult(const QString & result) {
            return result.isEmpty() ? OptionalText() : OptionalText(result.toStdString());
        }
        inline void message(QMessageBox::Icon icon, const std::string & text, const std::string & title) {
            QMessageBox dialog(icon, QString::fromStdString(title), QString::fromStdString(text), QMessageBox::Ok);
            dialog.exec();
        }
    }}
    inline OptionalText OpenDirectory(const std::string & title = "Open Directory", const std::string & directory = "") {
        return Qt::detail::fileResult(QFileDialog::getExistingDirectory(nullptr,
            QString::fromStdString(title), QString::fromStdString(directory)));
    }
    inline OptionalText OpenFile(const std::string & title = "Open File", const std::string & directory = "",
                                 const std::string & types = "") {
        return Qt::detail::fileResult(QFileDialog::getOpenFileName(nullptr,
            QString::fromStdString(title), QString::fromStdString(directory), Qt::detail::fileFilters(types)));
    }
    inline Optional<std::vector<std::string>> OpenFiles(const std::string & title = "Open Files",
            const std::string & directory = "", const std::string & types = "") {
        const auto paths = QFileDialog::getOpenFileNames(nullptr,
            QString::fromStdString(title), QString::fromStdString(directory), Qt::detail::fileFilters(types));
        if(paths.isEmpty()) return {};
        std::vector<std::string> result;
        for(const auto & path: paths) result.push_back(path.toStdString());
        return result;
    }
    inline OptionalText SaveFile(OptionalText defaultValue = {}, const std::string & title = "Save File",
                                  const std::string & directory = "", const std::string & types = "") {
        const auto path = directory.empty() ? defaultValue.valueOr("") : directory;
        return Qt::detail::fileResult(QFileDialog::getSaveFileName(nullptr,
            QString::fromStdString(title), QString::fromStdString(path), Qt::detail::fileFilters(types)));
    }
    inline OptionalText SaveFile(const std::string & defaultValue, const std::string & title = "Save File",
                                  const std::string & directory = "", const std::string & types = "") {
        return SaveFile(OptionalText(defaultValue), title, directory, types);
    }
    inline void Information(const std::string & message = "Information", const std::string & title = "Information Dialog") {
        Qt::detail::message(QMessageBox::Information, message, title);
    }
    inline void Warning(const std::string & message = "Warning", const std::string & title = "Warning Dialog") {
        Qt::detail::message(QMessageBox::Warning, message, title);
    }
    inline void Critical(const std::string & message = "Critical", const std::string & title = "Critical Dialog") {
        Qt::detail::message(QMessageBox::Critical, message, title);
    }
    inline bool Confirm(const std::string & message = "Confirm", const std::string & title = "Confirm Dialog") {
        QMessageBox dialog(QMessageBox::Question, QString::fromStdString(title), QString::fromStdString(message),
                           QMessageBox::Yes | QMessageBox::No);
        return dialog.exec() == QMessageBox::Yes;
    }
    inline OptionalText Prompt(const std::string & prompt = "Input", const std::string & title = "Input Dialog",
                               const std::string & defaultValue = "") {
        bool accepted = false;
        const auto text = QInputDialog::getText(nullptr, QString::fromStdString(title), QString::fromStdString(prompt),
            QLineEdit::Normal, QString::fromStdString(defaultValue), &accepted);
        return accepted ? OptionalText(text.toStdString()) : OptionalText();
    }
}
#endif

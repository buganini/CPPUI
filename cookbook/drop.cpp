#include "backend.hpp"
#include "recipes.hpp"
#include <fstream>
#include <sstream>

namespace Cookbook {
    void drop(Model & model) {
        for(auto column:CPPUI::VBox()) {
            CPPUI::Label("DROP HERE").style({}, {}, 36).drop([&model](CPPUI::DropEvent & event) {
                if(model.rejectDrop) { event.reject("Rejected by the example"); return; }
                std::ostringstream lines, contents; lines << "Action: " << event.action;
                for(const auto & file:event.files) {
                    struct stat info; const bool exists=::stat(file.path.c_str(),&info)==0;
                    if(exists && (info.st_mode&S_IFMT)==S_IFDIR) {
                        lines << "\n" << file.name << ": directory (" << file.path << ")"; continue;
                    }
                    const auto size=exists ? info.st_size : 0;
                    lines << "\n" << file.name << ": " << size << " bytes (temporary=" << (file.temporary ? "true" : "false") << ")";
                    const auto name=CPPUI::detail::extensionLower(file.name);
                    if(name.size()>=4 && name.compare(name.size()-4,4,".txt")==0) {
                        std::ifstream input(file.path); if(!input) throw std::runtime_error("Cannot read dropped text file");
                        contents << file.name << "\n" << input.rdbuf() << "\n\n";
                    }
                }
                if(event.text) lines << "\n" << *event.text;
                for(const auto & url:event.urls) lines << "\n" << url;
                model.dropResult=lines.str(); model.dropText=contents.str(); event.accept();
            }, {".txt", ".json", "/"});
            CPPUI::Checkbox("Reject this drop",model.rejectDrop);
            CPPUI::Label(static_cast<std::string>(model.dropResult));
            CPPUI::Label(static_cast<std::string>(model.dropText));
            CPPUI::Spacer();
        }
    }
}

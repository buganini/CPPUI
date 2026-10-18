#ifndef CPPUI_DROP
#define CPPUI_DROP

#include "optional.hpp"
#include <algorithm>
#include <functional>
#include <vector>
#include <sys/stat.h>

namespace CPPUI {
    struct DropFile {
        std::string name, path;
        bool temporary;
        DropFile(std::string name,std::string path,bool temporary=false):name(std::move(name)),path(std::move(path)),temporary(temporary) {}
    };
    struct DropEvent {
        std::vector<DropFile> files;
        OptionalText text;
        std::vector<std::string> urls;
        Optional<double> x, y;
        std::string action="copy";
        bool accepted=false;
        std::string reason="Drop was not accepted";
        void accept() { accepted=true; reason.clear(); }
        void reject(const std::string & why="Drop was rejected") { accepted=false; reason=why; }
    };
    namespace detail {
        inline std::string extensionLower(std::string value) {
            for(auto & c:value) if(c>='A' && c<='Z') c+= 'a'-'A';
            return value;
        }
        inline std::vector<std::string> dropExtensions(const std::vector<std::string> & source) {
            if(source.empty()) throw std::invalid_argument("extensions must contain suffixes such as .txt or .tar.gz");
            std::vector<std::string> result;
            for(auto value:source) {
                value=extensionLower(value);
                if(value.size()<2 || value[0]!='.' || value.find_first_of("/\\")!=std::string::npos || value.find('\0')!=std::string::npos)
                    throw std::invalid_argument("extensions must contain suffixes such as .txt or .tar.gz");
                if(std::find(result.begin(),result.end(),value)==result.end()) result.push_back(value);
            }
            return result;
        }
        struct DropHandlers {
            std::function<void(DropEvent &)> entered, dropped;
            Optional<std::vector<std::string>> extensions;
            void dispatch(DropEvent & event,bool enabled=true) const {
                if(!dropped || !enabled) { event.reject("Drop target is no longer available"); return; }
                std::string invalid;
                if(extensions) for(const auto & file:event.files) {
                    const auto name=extensionLower(file.name); bool matches=false;
                    for(const auto & suffix:*extensions)
                        if(name.size()>=suffix.size() && name.compare(name.size()-suffix.size(),suffix.size(),suffix)==0) matches=true;
                    if(!matches) { if(!invalid.empty()) invalid+=", "; invalid+=file.name; }
                }
                if(!invalid.empty()) { event.reject("Unsupported file extension: "+invalid); return; }
                for(const auto & file:event.files) {
                    struct stat info;
                    if(::stat(file.path.c_str(),&info)==0 && (info.st_mode&S_IFMT)==S_IFDIR) {
                        event.reject("Dropping directories is not supported"); return;
                    }
                }
                // Callers snapshot handlers; callbacks may remove the target.
                try { dropped(event); }
                catch(...) { event.reject("Drop handler failed"); throw; }
            }
        };
    }
}
#endif

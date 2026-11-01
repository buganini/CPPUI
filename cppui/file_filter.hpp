#ifndef CPPUI_FILE_FILTER
#define CPPUI_FILE_FILTER
#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>
namespace CPPUI {
    struct FileFilterGroup { std::string label; std::vector<std::string> extensions; };
    class FileFilters {
        std::vector<FileFilterGroup> entries;
        void normalize() {
            for(auto & group:entries) {
                if(group.extensions.empty()) throw std::invalid_argument("File extensions require a non-empty list");
                std::vector<std::string> unique;
                for(auto ext:group.extensions) {
                    if(!ext.empty() && ext[0]=='.') ext.erase(0,1);
                    if(ext!="*" && (ext.empty() || ext.find_first_of("*?[]/\\;|()")!=std::string::npos ||
                        std::any_of(ext.begin(),ext.end(),[](unsigned char c){return std::isspace(c); })))
                        throw std::invalid_argument("Invalid file extension");
                    if(std::find(unique.begin(),unique.end(),ext)==unique.end()) unique.push_back(ext);
                }
                group.extensions=std::move(unique);
            }
        }
    public:
        FileFilters() = default;
        FileFilters(std::initializer_list<std::string> extensions): FileFilters(std::vector<std::string>(extensions)) {}
        FileFilters(std::vector<std::string> extensions) {if(!extensions.empty()) entries.push_back({"Files",std::move(extensions)});normalize();}
        FileFilters(std::vector<FileFilterGroup> groups):entries(std::move(groups)) {normalize();}
        const std::vector<FileFilterGroup> & groups() const {return entries;}
        std::string native(bool qt) const {
            std::string result;
            for(const auto & group:entries) {
                if(!result.empty()) result+=qt ? ";;" : "|";
                result+=group.label;result+=qt ? " (" : "|";
                bool first=true;
                for(const auto & ext:group.extensions) {if(!first) result+=qt ? " " : ";";first=false;result+=ext=="*" ? "*" : "*."+ext;}
                if(qt) result+=")";
            }
            return result;
        }
    };
}
#endif

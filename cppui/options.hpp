#ifndef CPPUI_OPTIONS
#define CPPUI_OPTIONS

#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace CPPUI {
    namespace detail {
        inline bool validUtf8(const std::string & value) {
            for(std::size_t i=0;i<value.size();) {
                const unsigned char c=value[i++];
                if(c<0x80) continue;
                unsigned code=0, count=0, minimum=0;
                if(c>=0xc2 && c<=0xdf) { code=c&31; count=1; minimum=0x80; }
                else if(c>=0xe0 && c<=0xef) { code=c&15; count=2; minimum=0x800; }
                else if(c>=0xf0 && c<=0xf4) { code=c&7; count=3; minimum=0x10000; }
                else return false;
                if(i+count>value.size()) return false;
                while(count--) { const unsigned char next=value[i++]; if((next&0xc0)!=0x80) return false; code=(code<<6)|(next&63); }
                if(code<minimum || code>0x10ffff || (code>=0xd800 && code<=0xdfff)) return false;
            }
            return true;
        }
        inline int hexDigit(char c) {
            if(c>='0'&&c<='9') return c-'0';
            if(c>='a'&&c<='f') return c-'a'+10;
            if(c>='A'&&c<='F') return c-'A'+10;
            return -1;
        }
        inline std::string optionDecode(const std::string & source) {
            std::string result;
            for(std::size_t i=0;i<source.size();++i) {
                if(source[i]!='%') { result+=source[i]; continue; }
                if(i+2>=source.size() || hexDigit(source[i+1])<0 || hexDigit(source[i+2])<0)
                    throw std::invalid_argument("Expected percent-encoded name=value");
                result+=char((hexDigit(source[i+1])<<4)|hexDigit(source[i+2])); i+=2;
            }
            if(!validUtf8(result)) throw std::invalid_argument("Invalid UTF-8");
            return result;
        }
    }
    inline std::map<std::string,std::string> parseOptions(const std::string & source, std::ostream & warnings=std::cerr) {
        std::map<std::string,std::string> values;
        for(std::size_t offset=0;offset<source.size();) {
            const auto end=source.find('#',offset);
            const auto entry=source.substr(offset,end==std::string::npos ? end : end-offset);
            offset=end==std::string::npos ? source.size() : end+1;
            if(entry.empty()) continue;
            try {
                const auto equals=entry.find('=');
                if(equals==0 || equals==std::string::npos) throw std::invalid_argument("Expected percent-encoded name=value");
                const auto name=detail::optionDecode(entry.substr(0,equals));
                const auto value=detail::optionDecode(entry.substr(equals+1));
                values[name]=value;
            } catch(const std::invalid_argument & error) { warnings << "CPPUI: skipping invalid PUI_OPTIONS entry: " << error.what() << '\n'; }
        }
        return values;
    }
    // Lazy initialization is the header-only C++ equivalent of PUI's import.
    inline std::map<std::string,std::string> & Options() {
        static auto values=parseOptions(std::getenv("PUI_OPTIONS") ? std::getenv("PUI_OPTIONS") : "");
        return values;
    }
    inline void loadEnvironmentOptions() {
        Options()=parseOptions(std::getenv("PUI_OPTIONS") ? std::getenv("PUI_OPTIONS") : "");
    }
}
#endif

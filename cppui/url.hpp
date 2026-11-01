#ifndef CPPUI_URL
#define CPPUI_URL
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
namespace CPPUI { namespace detail {
    inline const std::string & validateURL(const std::string & url) {
        if(url.empty() || url.size()>8192 || std::isspace(static_cast<unsigned char>(url.front())) ||
           std::isspace(static_cast<unsigned char>(url.back())) || std::any_of(url.begin(),url.end(),[](unsigned char c){return c<32 || c==127;}))
            throw std::invalid_argument("OpenURL requires a nonempty URL without control characters");
        const auto colon=url.find(':');std::string scheme=url.substr(0,colon);
        std::transform(scheme.begin(),scheme.end(),scheme.begin(),[](unsigned char c){return std::tolower(c);});
        if(colon==std::string::npos || (scheme!="http" && scheme!="https" && scheme!="mailto"))
            throw std::invalid_argument("OpenURL supports absolute http, https and mailto URLs");
        if(scheme=="mailto") {if(colon+1==url.size() || url[colon+1]=='?' || url[colon+1]=='#') throw std::invalid_argument("OpenURL requires a mail recipient");}
        else {
            if(url.substr(colon+1,2)!="//") throw std::invalid_argument("OpenURL requires a host");
            auto host=url.substr(colon+3,url.find_first_of("/?#",colon+3)-(colon+3));
            const auto at=host.rfind('@');if(at!=std::string::npos) host.erase(0,at+1);
            if(host.find('[')!=std::string::npos || host.find(']')!=std::string::npos) {
                auto end=host.find(']');
                if(host.front()!='[' || end==std::string::npos || end<2 || (end+1<host.size() && host[end+1]!=':')) throw std::invalid_argument("OpenURL requires a valid bracketed host");
            }
            if(host.empty() || host.front()==':' || host=="[]") throw std::invalid_argument("OpenURL requires a host");
        }
        return url;
    }
}}
#endif

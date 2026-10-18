#include "cppui/drop.hpp"
#include <stdexcept>
static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
int main() {
    CPPUI::detail::DropHandlers handlers; int calls=0;
    handlers.extensions=CPPUI::detail::dropExtensions({".TXT",".json",".txt",".tar.gz"});
    require(handlers.extensions->size()==3,"suffixes did not normalize");
    handlers.dropped=[&](CPPUI::DropEvent & event){++calls; event.accept();};
    CPPUI::DropEvent event; event.files.emplace_back("VALUE.JSON","/tmp/missing.json");
    handlers.dispatch(event); require(event.accepted && calls==1,"case-insensitive extension failed");
    event=CPPUI::DropEvent(); event.files.emplace_back("archive.TAR.GZ","/tmp/missing.tar.gz"); handlers.dispatch(event);
    require(event.accepted && calls==2,"compound suffix failed");
    event=CPPUI::DropEvent(); event.files.emplace_back("value.exe","/tmp/missing.exe"); handlers.dispatch(event);
    require(!event.accepted && calls==2,"unsupported file invoked handler");
    event=CPPUI::DropEvent(); event.files.emplace_back("folder.txt","/tmp"); handlers.dispatch(event);
    require(!event.accepted && calls==2,"directory was accepted");
    event=CPPUI::DropEvent(); event.text=std::string("Text"); handlers.dispatch(event);
    require(event.accepted && calls==3,"file filter rejected text");
    event=CPPUI::DropEvent(); handlers.dispatch(event,false); require(!event.accepted && calls==3,"disabled target invoked handler");
    handlers.dropped=[](CPPUI::DropEvent &){}; event=CPPUI::DropEvent(); handlers.dispatch(event);
    require(!event.accepted,"drop accepted without explicit accept");
    handlers.dropped=[](CPPUI::DropEvent & e){e.accept();throw std::runtime_error("expected");};
    bool failed=false; try {handlers.dispatch(event);} catch(const std::runtime_error &) {failed=true;}
    require(failed && !event.accepted && event.reason=="Drop handler failed","handler failure kept acceptance");
    for(const auto & values:std::vector<std::vector<std::string>>{{},{"txt"},{"."},{".a/b"},{".a\\b"},{std::string(".a\0b",4)}}) {
        bool invalid=false; try {CPPUI::detail::dropExtensions(values);} catch(const std::invalid_argument &) {invalid=true;}
        require(invalid,"invalid suffix accepted");
    }
}

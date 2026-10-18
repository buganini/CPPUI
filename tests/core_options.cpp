#include "cppui/options.hpp"
#include <sstream>
int main() {
    std::ostringstream warnings;
    auto values=CPPUI::parseOptions("#a=b#a=last#empty=#count=00123#text=%E4%B8%96%E7%95%8C%23%3D#plus=a+b#equals=a=b#a=%zz#missing#=empty#bad=%FF#surrogate=%ED%A0%80#long=%C0%80#good=null",warnings);
    const std::map<std::string,std::string> expected{{"a","last"},{"empty",""},{"count","00123"},{"text","世界#="},{"plus","a+b"},{"equals","a=b"},{"good","null"}};
    if(values!=expected || warnings.str().empty()) throw std::runtime_error("environment option decoding failed");
    if(!CPPUI::parseOptions("").empty()) throw std::runtime_error("empty options were retained");
    if(CPPUI::parseOptions("%61=%00").at("a") != std::string(1,'\0')) throw std::runtime_error("escaped option bytes lost");
}

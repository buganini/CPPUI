#include "cppui/landlock.hpp"
#include <atomic>
#include <thread>
#include <sys/wait.h>
#include <cstring>
#include <fstream>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static void require(bool value,const char * message) { if(!value) throw std::runtime_error(message); }
static bool opens(const std::string & path,int flags) {int fd=::open(path.c_str(),flags,0600); if(fd>=0) {::close(fd);return true;}return false;}
static int child(const std::function<void()> & function) {
    const auto pid=::fork(); require(pid>=0,"fork failed");
    if(!pid) {try {function();::_exit(0);}catch(const std::exception & error){std::cerr<<error.what()<<'\n';::_exit(1);}}
    int status=0; require(::waitpid(pid,&status,0)==pid,"wait failed"); return WIFEXITED(status)?WEXITSTATUS(status):1;
}
int main() {
    try { CPPUI::landlockABI(); }
    catch(const std::exception & error) { std::cerr<<"Landlock unavailable: "<<error.what()<<'\n'; return 77; }
    char pattern[]="/tmp/cppui-landlock-XXXXXX"; auto created=::mkdtemp(pattern);
    require(created,"fixture creation failed"); const std::string root=created;
    for(auto name:{"read","write","outside"}) {
        require(::mkdir((root+"/"+name).c_str(),0700)==0,"fixture directory failed");
        std::ofstream file(root+"/"+name+"/file");file<<"original";
    }
    require(::symlink((root+"/outside/file").c_str(),(root+"/write/escape").c_str())==0,"symlink fixture failed");
    int result=0;
    result|=child([&](){
        CPPUI::LandlockPolicy policy; policy.read={root+"/read"}; policy.write={root+"/write"}; CPPUI::applyLandlock(policy);
        require(opens(root+"/read/file",O_RDONLY),"read grant failed");
        require(opens(root+"/write/file",O_WRONLY|O_TRUNC),"write/truncate grant failed");
        require(::rename((root+"/write/file").c_str(),(root+"/write/renamed").c_str())==0,"rename grant failed");
        require(::unlink((root+"/write/renamed").c_str())==0,"remove grant failed");
        require(!opens(root+"/read/file",O_WRONLY|O_TRUNC),"read-only rule allowed write");
        require(!opens(root+"/outside/file",O_RDONLY),"unlisted file was readable");
        require(!opens(root+"/write/escape",O_RDONLY),"symlink escaped hierarchy");
        require(child([&](){require(!opens(root+"/outside/file",O_RDONLY),"child did not inherit policy");})==0,"descendant test failed");
    });
    result|=child([&](){CPPUI::applyLandlock({}); require(!opens(root+"/read/file",O_RDONLY),"empty policy did not deny reads");});
    result|=child([&](){
        CPPUI::LandlockPolicy policy; policy.read={root+"/missing"}; bool failed=false;
        try {CPPUI::applyLandlock(policy);}catch(const std::exception &){failed=true;}
        require(failed && opens(root+"/outside/file",O_RDONLY),"invalid path applied partial policy");
    });
    result|=child([&](){
        std::atomic<bool> stop(false); std::thread thread([&](){while(!stop.load())std::this_thread::yield();});
        bool failed=false; try {CPPUI::applyLandlock({});}catch(const std::runtime_error &){failed=true;}
        stop=true; thread.join(); require(failed,"policy accepted preexisting threads");
    });
    ::unlink((root+"/write/escape").c_str());
    for(auto name:{"read","write","outside"}) {::unlink((root+"/"+name+"/file").c_str());::rmdir((root+"/"+name).c_str());}
    ::rmdir(root.c_str()); return result;
}

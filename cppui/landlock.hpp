#ifndef CPPUI_LANDLOCK
#define CPPUI_LANDLOCK

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#if defined(__linux__) && defined(__has_include)
#if __has_include(<linux/landlock.h>)
#define CPPUI_HAS_LANDLOCK_HEADERS 1
#include <linux/landlock.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <cstdlib>
#include <cerrno>
#include <memory>
#endif
#endif

namespace CPPUI {
    struct LandlockPolicy {
        std::vector<std::string> read, write, execute;
    };
#if defined(CPPUI_HAS_LANDLOCK_HEADERS) && defined(SYS_landlock_create_ruleset)
    namespace detail {
        struct LandlockFD {
            int value;
            explicit LandlockFD(int value):value(value) { if(value<0) throw std::system_error(errno,std::generic_category(),"Landlock"); }
            ~LandlockFD() { ::close(value); }
            LandlockFD(const LandlockFD &)=delete;
            LandlockFD & operator=(const LandlockFD &)=delete;
        };
        inline void singleLandlockThread() {
            std::unique_ptr<DIR,int(*)(DIR *)> directory(::opendir("/proc/self/task"),::closedir);
            if(!directory) throw std::system_error(errno,std::generic_category(),"Landlock thread check");
            unsigned count=0;
            while(auto entry=::readdir(directory.get())) if(entry->d_name[0]!='.') ++count;
            if(count!=1) throw std::runtime_error("Apply Landlock before starting background threads");
        }
    }
    inline int landlockABI() {
        const auto abi=::syscall(SYS_landlock_create_ruleset,nullptr,0,LANDLOCK_CREATE_RULESET_VERSION);
        if(abi<3) throw std::runtime_error("Landlock requires kernel ABI 3 or newer (including truncate protection)");
        return static_cast<int>(abi);
    }
    inline void applyLandlock(const LandlockPolicy & policy) {
        const int abi=landlockABI();
        detail::singleLandlockThread();
        // ABI 3's filesystem rights, plus device ioctl restrictions on ABI 5+.
        const std::uint64_t truncate=1ULL<<14, ioctlDevice=1ULL<<15;
        const std::uint64_t handled=((1ULL<<15)-1) | (abi>=5 ? ioctlDevice : 0);
        const std::uint64_t read=LANDLOCK_ACCESS_FS_READ_FILE|LANDLOCK_ACCESS_FS_READ_DIR;
        const std::uint64_t files=LANDLOCK_ACCESS_FS_EXECUTE|LANDLOCK_ACCESS_FS_WRITE_FILE|
            LANDLOCK_ACCESS_FS_READ_FILE|truncate|(abi>=5 ? ioctlDevice : 0);
        struct landlock_ruleset_attr rules={}; rules.handled_access_fs=handled;
        detail::LandlockFD ruleset(::syscall(SYS_landlock_create_ruleset,&rules,sizeof(rules),0));
        const std::vector<std::string> * groups[]={&policy.read,&policy.write,&policy.execute};
        const std::uint64_t rights[]={read,handled&~LANDLOCK_ACCESS_FS_EXECUTE,LANDLOCK_ACCESS_FS_EXECUTE};
        for(unsigned i=0;i<3;++i) for(const auto & path:*groups[i]) {
            if(path.empty() || path.find('\0')!=std::string::npos) throw std::invalid_argument("Landlock paths must name existing files or directories");
            std::unique_ptr<char,void(*)(void *)> resolved(::realpath(path.c_str(),nullptr),::free);
            if(!resolved) throw std::system_error(errno,std::generic_category(),"Landlock path");
            detail::LandlockFD parent(::open(resolved.get(),O_PATH|O_CLOEXEC));
            struct stat info;
            if(::fstat(parent.value,&info)<0) throw std::system_error(errno,std::generic_category(),"Landlock path type");
            struct landlock_path_beneath_attr rule={};
            rule.parent_fd=parent.value; rule.allowed_access=S_ISDIR(info.st_mode) ? rights[i] : rights[i]&files;
            if(::syscall(SYS_landlock_add_rule,ruleset.value,LANDLOCK_RULE_PATH_BENEATH,&rule,0)<0)
                throw std::system_error(errno,std::generic_category(),"Landlock rule");
        }
        if(::prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)<0 || ::syscall(SYS_landlock_restrict_self,ruleset.value,0)<0)
            throw std::system_error(errno,std::generic_category(),"Landlock restriction");
    }
#else
    inline int landlockABI() { throw std::runtime_error("Landlock is unavailable in this build"); }
    inline void applyLandlock(const LandlockPolicy &) {
#ifdef __linux__
        throw std::runtime_error("Landlock is unavailable in this build; Linux Landlock headers and syscall definitions are required");
#else
        std::cerr << "CPPUI: Landlock is only available on Linux; ignoring policy\n";
#endif
    }
#endif
}
#endif

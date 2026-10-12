#ifndef CPPUI_WX_IMAGE
#define CPPUI_WX_IMAGE
#include "base.hpp"
#include "image_resource.hpp"
#include <wx/statbmp.h>
#include <sys/stat.h>
namespace CPPUI { namespace Wx {
class Image: public Widget {
    std::string path;
    struct stat stamp{};
    ImageResource source;
public:
    explicit Image(const std::string & path):path(path) {
        if(stat(path.c_str(),&stamp)!=0)throw std::runtime_error("Image file does not exist: "+path);
    }
    void updateWidget(Widget * previous) override {
        auto prev=static_cast<Image *>(previous);
        if(prev && prev->path==path && prev->stamp.st_mtime==stamp.st_mtime && prev->stamp.st_size==stamp.st_size
#if defined(__linux__)
            && prev->stamp.st_mtim.tv_nsec==stamp.st_mtim.tv_nsec
#elif defined(__APPLE__)
            && prev->stamp.st_mtimespec.tv_nsec==stamp.st_mtimespec.tv_nsec
#endif
        ) source=prev->source;
        else source=ImageResource::load(path);
        if(source.isNull())throw std::runtime_error("Cannot load image: "+path);
        auto displayed=source;
        if(layout_width || layout_height) {
            const double ratio=std::min(layout_width?double(*layout_width)/source.width():1e30,layout_height?double(*layout_height)/source.height():1e30);
            displayed=source.scale(std::max(1,int(source.width()*ratio)),std::max(1,int(source.height()*ratio)),false,1);
        }
        ui=prev?prev->ui:new wxStaticBitmap(parentWindow(),wxID_ANY,wxNullBitmap);
        static_cast<wxStaticBitmap *>(ui)->SetBitmap(wxBitmap(displayed.nativeImage()));
    }
    void bindEvents() override {ui->Bind(wxEVT_LEFT_DOWN,&Native::mouse,native.get());}
};
}
inline Wx::Image & Image(const std::string & path){return *new Wx::Image(path);}
}
#endif

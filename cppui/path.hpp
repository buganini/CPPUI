#ifndef CPPUI_PATH
#define CPPUI_PATH
#include <cmath>
#include <initializer_list>
#include <stdexcept>
#include <vector>
namespace CPPUI {
    enum class PathOp {MoveTo,LineTo,QuadTo,CubicTo,Close};
    enum class FillRule {NonZero,EvenOdd};
    struct PathCommand {
        PathOp op;
        std::vector<double> values;
        PathCommand(PathOp op,std::initializer_list<double> values={}):op(op),values(values) {}
    };
    using CanvasPath=std::vector<PathCommand>;
    namespace detail {
        inline void validatePath(const CanvasPath & path,FillRule rule) {
            if(rule!=FillRule::NonZero && rule!=FillRule::EvenOdd) throw std::invalid_argument("Invalid fill rule");
            if(path.size()>20000 || (!path.empty() && path.front().op!=PathOp::MoveTo)) throw std::invalid_argument("Invalid Canvas path");
            for(const auto & cmd:path) {
                int count;
                switch(cmd.op) {
                    case PathOp::MoveTo:case PathOp::LineTo:count=2;break;
                    case PathOp::QuadTo:count=4;break;
                    case PathOp::CubicTo:count=6;break;
                    case PathOp::Close:count=0;break;
                    default:throw std::invalid_argument("Invalid path operation");
                }
                if(cmd.values.size()!=std::size_t(count)) throw std::invalid_argument("Invalid path coordinates");
                for(double v:cmd.values) if(!std::isfinite(v) || std::abs(v)>1e9) throw std::invalid_argument("Invalid path coordinate");
            }
        }
    }
}
#endif

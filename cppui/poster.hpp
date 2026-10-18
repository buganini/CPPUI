#ifndef CPPUI_POSTER
#define CPPUI_POSTER
#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>

namespace CPPUI {
    using Callback = std::function<void()>;
    using CommandPoster = std::function<void(Callback)>;
    // Called on the UI thread, with positive integral milliseconds.
    using TimerPoster = std::function<void(int, Callback)>;
    class Poster {
        using Clock = std::chrono::steady_clock;
        static double now() { return std::chrono::duration<double,std::milli>(Clock::now().time_since_epoch()).count(); }
        struct Pending: std::enable_shared_from_this<Pending> {
            double deadline;
            Callback callback;
            TimerPoster later;
            void run() {
                const auto remaining=deadline-now();
                if(remaining<=0) { auto invoke=std::move(callback); invoke(); }
                else {
                    auto self=shared_from_this();
                    const int milliseconds=static_cast<int>(std::min(std::ceil(remaining),double(std::numeric_limits<int>::max())));
                    later(milliseconds,[self]() { self->run(); });
                }
            }
        };
        CommandPoster submit;
        TimerPoster later;
    public:
        Poster() = default;
        Poster(CommandPoster submit,TimerPoster later):submit(std::move(submit)),later(std::move(later)) {}
        void operator()(Callback callback,double delay=0) const {
            if(!callback) throw std::invalid_argument("Posted callback must not be empty");
            if(!std::isfinite(delay) || delay<0) throw std::invalid_argument("Post delay must be finite and non-negative (milliseconds)");
            if(!submit || !later) throw std::logic_error("Poster has no UI dispatcher");
            if(delay==0) { submit(std::move(callback)); return; }
            auto pending=std::make_shared<Pending>();
            pending->deadline=now()+delay; pending->callback=std::move(callback); pending->later=later;
            submit([pending]() { pending->run(); });
        }
    };
}
#endif

#ifndef CPPUI_OPTIONAL
#define CPPUI_OPTIONAL

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace CPPUI {
    // C++11 value semantics; an absent value does not construct a T.
    template<class T>
    class Optional {
        std::unique_ptr<T> data;
        public:
        Optional() = default;
        Optional(const T & value): data(new T(value)) {}
        Optional(T && value): data(new T(std::move(value))) {}
        Optional(const Optional & other): data(other ? new T(*other) : nullptr) {}
        Optional(Optional &&) noexcept = default;
        Optional & operator=(Optional other) noexcept { data.swap(other.data); return *this; }
        bool hasValue() const noexcept { return bool(data); }
        explicit operator bool() const noexcept { return hasValue(); }
        T & value() {
            if(!data) throw std::logic_error("Optional has no value");
            return *data;
        }
        const T & value() const {
            if(!data) throw std::logic_error("Optional has no value");
            return *data;
        }
        T & operator*() { return value(); }
        const T & operator*() const { return value(); }
        T * operator->() { return &value(); }
        const T * operator->() const { return &value(); }
        T valueOr(const T & fallback) const { return data ? *data : fallback; }
        void reset() noexcept { data.reset(); }
    };
    using OptionalText = Optional<std::string>;
}
#endif

#ifndef CPPUI_OPTIONAL
#define CPPUI_OPTIONAL

#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace CPPUI {
    // C++11 inline storage; an absent value does not construct a T.
    template<class T>
    class Optional {
        union Storage {
            char empty;
            T value;
            Storage() noexcept: empty() {}
            ~Storage() {}
        } storage;
        bool present = false;

        using Transfer = decltype(std::move_if_noexcept(std::declval<T &>()));
        template<class U>
        void construct(U && value) noexcept(std::is_nothrow_constructible<T, U &&>::value) {
            // Use global placement new and addressof even if T overloads them.
            auto address = const_cast<void *>(static_cast<const volatile void *>(std::addressof(storage.value)));
            ::new (address) T(std::forward<U>(value));
            present = true;
        }

        public:
        Optional() noexcept = default;
        Optional(const T & value) noexcept(std::is_nothrow_copy_constructible<T>::value) { construct(value); }
        Optional(T && value) noexcept(std::is_nothrow_move_constructible<T>::value) { construct(std::move(value)); }
        Optional(const Optional & other) noexcept(std::is_nothrow_copy_constructible<T>::value) {
            if(other) construct(*other);
        }
        Optional(Optional && other) noexcept(std::is_nothrow_constructible<T, Transfer>::value) {
            if(other) {
                construct(std::move_if_noexcept(*other));
                other.reset();
            }
        }
        ~Optional() { reset(); }
        Optional & operator=(Optional other) noexcept(std::is_nothrow_constructible<T, Transfer>::value) {
            // The incoming value is built before resetting this one. If the
            // final transfer throws, this Optional stays valid and empty.
            reset();
            if(other) construct(std::move_if_noexcept(*other));
            return *this;
        }
        bool hasValue() const noexcept { return present; }
        explicit operator bool() const noexcept { return hasValue(); }
        T & value() {
            if(!present) throw std::logic_error("Optional has no value");
            return storage.value;
        }
        const T & value() const {
            if(!present) throw std::logic_error("Optional has no value");
            return storage.value;
        }
        T & operator*() { return value(); }
        const T & operator*() const { return value(); }
        T * operator->() { return std::addressof(value()); }
        const T * operator->() const { return std::addressof(value()); }
        T valueOr(const T & fallback) const { return present ? storage.value : fallback; }
        void reset() noexcept {
            if(present) {
                present = false;
                storage.value.~T();
            }
        }
    };
    using OptionalText = Optional<std::string>;
}
#endif

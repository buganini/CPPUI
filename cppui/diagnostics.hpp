#ifndef CPPUI_DIAGNOSTICS
#define CPPUI_DIAGNOSTICS

// Annotate fallback factories, so warnings appear at use sites rather than
// whenever a backend header is included. Keep support for C++11 compilers.
#if defined(_MSC_VER)
#define CPPUI_UNSUPPORTED(message) __declspec(deprecated(message))
#elif defined(__clang__) || defined(__GNUC__)
#define CPPUI_UNSUPPORTED(message) __attribute__((deprecated(message)))
#elif __cplusplus >= 201402L
#define CPPUI_UNSUPPORTED(message) [[deprecated(message)]]
#else
#define CPPUI_UNSUPPORTED(message)
#endif

#endif

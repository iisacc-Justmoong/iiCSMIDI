#pragma once
#if defined(_WIN32) && !defined(IICSMIDI_STATIC)
# if defined(IICSMIDI_BUILDING_LIBRARY)
#    define IICSMIDI_EXPORT __declspec(dllexport)
# else
#    define IICSMIDI_EXPORT __declspec(dllimport)
# endif
#elif defined(__GNUC__) || defined(__clang__)
#    define IICSMIDI_EXPORT __attribute__((visibility("default")))
#else
#    define IICSMIDI_EXPORT
#endif


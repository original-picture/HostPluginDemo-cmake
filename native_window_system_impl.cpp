
#include "native_window_system.h"
#include "juce_gui_extra/juce_gui_extra.h"

#if __has_include("unistd.h")
    #if defined(__APPLE__) && defined(__MACH__)
        #include "native_window_system_impls/nswindow.cpp"
    #elif __has_include("X11/Xlib.h")
        #include "native_window_system_impls/x11.cpp"
    #else
        #include "native_window_system_impls/dummy_fallback.cpp"
    #endif

#elif defined(__WIN32__) || defined(_WIN32)
    #include "native_window_system_impls/win32.cpp"
#else
    #include "native_window_system_impls/dummy_fallback.cpp"
#endif
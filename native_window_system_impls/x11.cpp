

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/XShm.h>

#include <juce_gui_basics/juce_gui_basics.h>
#include "juce_gui_basics/native/juce_XSymbols_linux.h"


// Error handler for X11 errors
static int xErrorHandler(::Display* d, XErrorEvent* e) {
    char error_msg[256];
    // Use JUCE's X11Symbols to get error text
    juce::X11Symbols::getInstance()->xGetErrorText(d, e->error_code, error_msg, sizeof(error_msg));
    std::cerr << "X11 Error: " << error_msg << " (code: " << (unsigned)e->error_code
              << ", resource: " << e->resourceid << ")" << std::endl;
    return 0;
}

// Set the error handler (call in editor's constructor)
void set_handler() {
    //juce::X11Symbols::getInstance()->xSetErrorHandler(xErrorHandler);
}

void set_component_native_owning_window(juce::Component& to_be_owned, juce::Component& to_be_owner) {
    // Use JUCE's display to ensure consistency
    ::Display* display = juce::X11Symbols::getInstance()->xOpenDisplay(nullptr);
    if (!display) {
        std::cerr << "Failed to open X11 display" << std::endl;
        return;
    }

    // Get window handles from JUCE components
    Window to_be_owned_window = reinterpret_cast<Window>(to_be_owned.getWindowHandle());
    Window to_be_owner_window = reinterpret_cast<Window>(to_be_owner.getWindowHandle());

    // Validate window handles
    XWindowAttributes attrs;
    if (!juce::X11Symbols::getInstance()->xGetWindowAttributes(display, to_be_owned_window, &attrs)) {
        std::cerr << "Invalid to_be_owned window: " << to_be_owned_window << std::endl;
        juce::X11Symbols::getInstance()->xCloseDisplay(display);
        return;
    }
    if (!juce::X11Symbols::getInstance()->xGetWindowAttributes(display, to_be_owner_window, &attrs)) {
        std::cerr << "Invalid to_be_owner window: " << to_be_owner_window << std::endl;
        juce::X11Symbols::getInstance()->xCloseDisplay(display);
        return;
    }

    // Set the to_be_owned window as transient for the to_be_owner window
    Status result = juce::X11Symbols::getInstance()->xSetTransientForHint(display, to_be_owned_window, to_be_owner_window);
    // Flush errors to ensure the handler is called
    juce::X11Symbols::getInstance()->xSync(display, False);

    if (result == 0) {
        std::cerr << "XSetTransientForHint failed" << std::endl;
    } else {
        std::cout << "XSetTransientForHint succeeded" << std::endl;
    }

    // Clean up
    //juce::X11Symbols::getInstance()->xCloseDisplay(display);
}
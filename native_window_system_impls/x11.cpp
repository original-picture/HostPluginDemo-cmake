
#include "X11/Xlib.h"

#include <string>

std::string get_x_error(int code) {
    std::string ret(1024, ' ');

    XGetErrorText(XOpenDisplay(nullptr), code, ret.data(), 1024);

    return ret;
}

int handler(Display * d, XErrorEvent * e)
{
    return 0; // the handler does nothing, I just set a breakpoint in it to see if it gets called. It never does
}

void set_handler() {
    XSetErrorHandler(handler);
}

void set_component_native_owning_window(juce::Component& to_be_owned, juce::Component& to_be_owner) {

    auto display = XOpenDisplay(nullptr);

    if(display) {
        auto to_be_owned_window = reinterpret_cast<Window>(to_be_owned.getWindowHandle()),
             to_be_owner_window = reinterpret_cast<Window>(to_be_owner.getWindowHandle());

        if(int ret = XSetTransientForHint(display, to_be_owner_window,
                                                   to_be_owned_window)) { // failure
            //to_be_owned.setAlwaysOnTop(true); // doesn't do anything on linux

            auto str = get_x_error(ret);
        }
    }
    else { // XOpenDisplay failed

    }
}
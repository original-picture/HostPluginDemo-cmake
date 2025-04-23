
#include "X11/Xlib.h"
#include "X11/X.h"
#include "X11/Xutil.h"
#include "X11/Xcursor/Xcursor.h"
#include "X11/extensions/Xinerama.h"
#include "X11/extensions/Xrandr.h"
#include "X11/extensions/XShm.h"

#include "juce_gui_basics/native/juce_XSymbols_linux.h"

#include <string>

std::string get_x_error(int code) {
    std::string ret(1024, ' ');

    juce::X11Symbols::getInstance()->xGetErrorText(juce::X11Symbols::getInstance()->xOpenDisplay(nullptr), code, ret.data(), 1024);

    return ret;
}

int handler(Display * d, XErrorEvent * e)
{

}

void set_handler() {
    juce::X11Symbols::getInstance()->xSetErrorHandler(handler);
}

void set_component_native_owning_window(juce::Component& to_be_owned, juce::Component& to_be_owner) {

    int ret;
    auto display = juce::X11Symbols::getInstance()->xOpenDisplay(nullptr);



    if(display) {
        if(ret = juce::X11Symbols::getInstance()->xSetTransientForHint(display,
                reinterpret_cast<Window>(to_be_owned.getWindowHandle()),
                reinterpret_cast<Window>(to_be_owner.getWindowHandle()))) { // failure
            //to_be_owned.setAlwaysOnTop(true); // fallback behavior. Not ideal but close enough to what we want

            auto str = get_x_error(ret);

        }
    }
    else {

    }
}
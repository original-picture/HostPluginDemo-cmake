
#include "X11/Xlib.h"

void set_component_native_owning_window(juce::Component& to_be_owned, juce::Component& to_be_owner) {
   /* if(XSetTransientForHint(XOpenDisplay(nullptr),
                         reinterpret_cast<Window>(to_be_owned.getWindowHandle()),
                         reinterpret_cast<Window>(to_be_owner.getWindowHandle()))) { // failure
       // to_be_owned.setAlwaysOnTop(true); // fallback behavior. Not ideal but close enough to what we want
    }*/
}
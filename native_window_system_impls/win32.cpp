

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void set_component_native_parent_window(juce::Component& child, juce::Component& parent) {
    if(SetParent(reinterpret_cast<HWND>(child.getWindowHandle()), reinterpret_cast<HWND>(parent.getWindowHandle()))) {

    }
    else {
        child.setAlwaysOnTop(true);
    }
}
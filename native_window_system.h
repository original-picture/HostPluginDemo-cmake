
#pragma once

#include "juce_gui_basics/juce_gui_basics.h"
#include "juce_gui_extra/juce_gui_extra.h"

// technically parent and child could be const references (getWindowHandle() is const),
// but I feel like (re)parenting a window is logically a mutation, so it makes sense to force the user to provide a mutable reference
void set_component_native_owning_window(juce::Component& child, juce::Component& parent);
void set_handler();


/// this file is used on unsupported platforms
/// the function implementations just have some kind of default behavior (sometimes they do nothing)
/// this is fine because none of the window related functionality is essential

void set_component_native_child_window(juce::Component& parent, juce::Component& child) {
    child.setAlwaysOnTop(true); // just put it on top. Not really ideal but similar enough to the desired behavior
}
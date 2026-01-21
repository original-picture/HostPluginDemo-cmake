
#pragma once

class plugin_picker : public juce::Component {
    // plugin_picker(std::function<void(juce::AudioProcessor
}

class plugin_picker_window : public juce::DocumentWindow {
public:
    std::unique_ptr<plugin_picker> pp;
};

class hosted_plugin_manager {
    std::unordered_map<juce::AudioPluginInstance*, std::unique_ptr<juce::AudioPluginInstance>>;

    std::unique_ptr<plugin_picker>        open_plugin_picker              (std::function<void(juce::AudioPluginInstance&)> plugin_picked_callback);
    std::unique_ptr<plugin_picker_window> open_plugin_picker_in_new_window(std::function<void(juce::AudioPluginInstance&)> plugin_picked_callback);|

    void juce::MemoryBlock get_state_information();
    void 		           set_state_information(const juce::MemoryBlock& mb);

    void set_scale_factor(float scale_factor);
};

/// checks if a->b->c is a valid signal chain
bool is_buses_layout_supported(const juce::AudioProcessor& a, bool a_is_host_processor,
                               const juce::AudioProcessor& b,
                               const juce::AudioProcessor& c, bool c_is_host_processor);

class default_editor : juce::Component {
public:
    default_editor(AudioPluginInstance* plugin_instance);

private:
    AudioPluginInstance* plugin_instance_;

    struct {
        juce::Label parameter_label;
        juce::Slider parameter_slider;
    } parameter_slider_;

    std::vector<parameter_slider_> sliders_;
};

class editor_container {
public:
    editor_container();

private:
    juce::ToggleButton ui_toggle_,
                       bypass_toggle_;

    juce::Slider mix_slider_;

    juce::Component editor_;
};

class suspend_processing_guard {
public:
    suspend_processing_guard(juce::AudioProcessor& ap);

    suspend_processing_guard(const suspend_processing_guard& ) = delete;
    suspend_processing_guard(      suspend_processing_guard&&) = delete;

    suspend_processing_guard& operator=(const suspend_processing_guard& ) = delete;
    suspend_processing_guard& operator=(      suspend_processing_guard&&) = delete;

    ~suspend_processing_guard();

private:
    juce::AudioProcessor* ap_;
};
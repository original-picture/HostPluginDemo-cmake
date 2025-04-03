#pragma once

#include <juce_audio_processors/juce_audio_processors.h>


//using namespace juce;

enum class EditorStyle { thisWindow, newWindow };


//==============================================================================
class HostAudioProcessorImpl : public  juce::AudioProcessor,
                               private juce::ChangeListener
{
public:
    HostAudioProcessorImpl();

    bool isBusesLayoutSupported (const BusesLayout& layouts) const final;
    void prepareToPlay (double sr, int bs) final;
    void releaseResources() final;
    void reset() final;

    // In this example, we don't actually pass any audio through the inner processor.
    // In a 'real' plugin, we'd need to add some synchronisation to ensure that the inner
    // plugin instance was never modified (deleted, replaced etc.) during a call to processBlock.
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) final;

    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) final;

    inline bool hasEditor() const override                                   { return false; }
    inline juce::AudioProcessorEditor* createEditor() override                     { return nullptr; }

    inline const juce::String getName() const final                                { return "HostPluginDemo-cmake"; }
    inline bool acceptsMidi() const final                                    { return true; }
    inline bool producesMidi() const final                                   { return true; }
    inline double getTailLengthSeconds() const final                         { return 0.0; }

    inline int getNumPrograms() final                                        { return 0; }
    inline int getCurrentProgram() final                                     { return 0; }
    inline void setCurrentProgram (int) final                                {}
    inline const juce::String getProgramName (int) final                           { return "None"; }
    inline void changeProgramName (int, const juce::String&) final                 {}

    void getStateInformation (juce::MemoryBlock& destData) final;
    void setStateInformation (const void* data, int sizeInBytes) final;

    void setNewPlugin (const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb = {});
    void clearPlugin();
    bool isPluginLoaded() const;

    std::unique_ptr<juce::AudioProcessorEditor> createInnerEditor() const;

    inline EditorStyle getEditorStyle() const noexcept { return editorStyle; }

    juce::ApplicationProperties appProperties;
    juce::AudioPluginFormatManager pluginFormatManager;
    juce::KnownPluginList pluginList;
    std::function<void()> pluginChanged;

private:
    juce::CriticalSection innerMutex;
    //std::unique_ptr<juce::AudioPluginInstance> inner;
    std::unique_ptr<juce::AudioPluginInstance> inner_ping_pong[2];
    bool current_ping_pong_index = 0;
    std::atomic<bool> plugin_changed = false;

    EditorStyle editorStyle = EditorStyle{};
    bool active = false;
    juce::ScopedMessageBox messageBox;

    static constexpr const char* innerStateTag = "inner_state";
    static constexpr const char* editorStyleTag = "editor_style";

    void changeListenerCallback (juce::ChangeBroadcaster* source) final;
};

class HostAudioProcessor final : public HostAudioProcessorImpl
{
public:
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
};

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>


//using namespace juce;

enum class EditorStyle { thisWindow, newWindow };

class HostAudioProcessorEditor;

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


    void set_plugin_changed_flag();

    void swap_active_inactive();


    inline       std::unique_ptr<juce::AudioPluginInstance>& inactive_inner()       { return inner_ping_pong[!active_ping_pong_index_];  }
    inline const std::unique_ptr<juce::AudioPluginInstance>& inactive_inner() const { return inner_ping_pong[!active_ping_pong_index_];  }

    inline       std::unique_ptr<juce::AudioPluginInstance>& active_inner()         { return inner_ping_pong[active_ping_pong_index_]; }
    inline const std::unique_ptr<juce::AudioPluginInstance>& active_inner()   const { return inner_ping_pong[active_ping_pong_index_]; }

private:
    juce::CriticalSection innerMutex; // I don't understand why this is necessary, because this mutex only ever gets locked on the message thread
                                      // maybe they were planning on locking it in processBlock() (on the audio thread), which would make sense functionally, because things like the inner plugin changing need to be synchronized
                                      // but mutexes really shouldn't be used on the audio thread to begin with, so idk --original-picture

    //std::unique_ptr<juce::AudioPluginInstance> inner;
    std::unique_ptr<juce::AudioPluginInstance> inner_ping_pong[2] = {nullptr, nullptr}; // my lock-free solution for synchronizing plugin changes is to hold two AudioPluginInstances and ping pong between them using an atomic index
                                                                                                // that one plugin can be getting loaded on the message thread while the other is being used on the audio thread and they don't mess with each other's data
                                                                                                // --original-picture

    std::atomic<unsigned char> active_ping_pong_index_ = 0; // I would have used a bool for this variable (because the index can only ever be 0 or 1),
                                                            // but I need to atomically flip it and std::atomic<bool> has no atomic negation operation
                                                            // so instead I use unsigned char and flip it by atomically xoring it with 1






    EditorStyle editorStyle = EditorStyle{};
    bool active = false; // I don't know what this does --original-picture
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

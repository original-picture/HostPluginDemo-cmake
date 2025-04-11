#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "forwarding_parameter_ptr.h"

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

    void swap_read_write();


    inline       std::unique_ptr<juce::AudioPluginInstance>& editor_write_inner()       { return inner_ping_pong[!active_ping_pong_index_];  }
    inline const std::unique_ptr<juce::AudioPluginInstance>& editor_write_inner() const { return inner_ping_pong[!active_ping_pong_index_];  }

    inline       std::unique_ptr<juce::AudioPluginInstance>& processor_read_inner()         { return inner_ping_pong[active_ping_pong_index_]; }
    inline const std::unique_ptr<juce::AudioPluginInstance>& processor_read_inner()   const { return inner_ping_pong[active_ping_pong_index_]; }

    /// this is all just ideas. I haven't hooked any of this up yet. So it might be completely wrong
    std::atomic<bool> inside_process_block; // basically a spinlock
    std::atomic<bool> plugin_already_changed_in_this_process_block_call; // this warrants some explanation
                                                                         // Long story short, it's possible for swap_read_write to get called while processBlock is still using processor_read_inner()
                                                                         // this isn't an issue on its own (doesn't cause a data race), because the usage of memory_order_release ensures that by the time the swapping atomic write is visible
                                                                         // the new plugin is no longer being written to
                                                                         // However, it does mean that, for the remainder of that process call, processBlock is technically reading old data
                                                                         // again, on its own this doesn't cause a data race. But it does become an issue if another plugin gets loaded in that same processBlock call
                                                                         // In that case, processBlock will be reading from editor_write_inner() while the editor is writing to it, causing a data race
                                                                         // the solution I came up with for this issue is to set inside_process_block to true at the beginning of processBlock and false at the end,
                                                                         // and set plugin_already_changed_in_this_process_block_call to true in swap_read_write and false also at the end of processBlock
                                                                         // then, before trying to modify editor_write_inner(), the editor will check to see if(inside_process_block&&plugin_already_changed_in_this_process_block_call)
                                                                         // and if it evaluates to true it will return from whatever function was being called without modifying editor_write_inner()
                                                                         // I think this is an okay solution because I don't think it's physically possible for a user to interact with the gui fast enough to load multiple plugins within one processBlock call,
                                                                         // (maybe it could happen if the gui freezes or something and a bunch of changes get reported to the processor at almost the same time)
                                                                         // so this early out path will likely never be hit

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


    static constexpr std::size_t maximum_number_of_parameters_ = 64;

    std::vector<forwarding_parameter_ptr*> parameters_;



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

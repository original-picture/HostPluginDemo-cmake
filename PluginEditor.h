#pragma once

#include "PluginProcessor.h"

//#include "juce_audio_processors/format_types/juce_VST3Headers.h"

//#include "juce_audio_processors/format_types/juce_VST3PluginFormat.h"

//space juce;

//==============================================================================
constexpr auto margin = 10;


static void doLayout (juce::Component* main, juce::Component& bottom, int bottomHeight, juce::Rectangle<int> bounds);


class PluginLoaderComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginLoaderComponent (juce::AudioPluginFormatManager& manager,
                           juce::KnownPluginList& list,
                           Callback&& callback)
            : pluginListComponent (manager, list, {}, {})
    {
        pluginListComponent.getTableListBox().setMultipleSelectionEnabled (false);

        addAndMakeVisible (pluginListComponent);
        addAndMakeVisible (buttons);

        const auto getCallback = [this, &list, cb = std::forward<Callback> (callback)] (EditorStyle style)
        {
            return [this, &list, cb, style]
            {
                const auto index = pluginListComponent.getTableListBox().getSelectedRow();
                const auto& types = list.getTypes();

                if (juce::isPositiveAndBelow (index, types.size()))
                    juce::NullCheckedInvocation::invoke (cb, types.getReference (index), style);
            };
        };

        buttons.thisWindowButton.onClick = getCallback (EditorStyle::thisWindow);
        buttons.newWindowButton .onClick = getCallback (EditorStyle::newWindow);
                                     // stupid dumb poltergeist type
        //juce::VST3PluginFormat vst3_plugin_format;
        //pluginListComponent.scanFor(vst3_plugin_format);
    }

    void resized() override;

private:
    struct Buttons final : public Component
    {
        Buttons();

        void resized() override;

        juce::Label label { "", "Select a plugin from the list, then display it using the buttons below." };
        juce::TextButton thisWindowButton { "Open In This Window" };
        juce::TextButton newWindowButton { "Open In New Window" };
    };

    juce::PluginListComponent pluginListComponent;
    Buttons buttons;
};


//==============================================================================
class PluginEditorComponent final : public juce::Component
{
public:
    template <typename Callback>
    PluginEditorComponent (std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose)
            : editor (std::move (editorIn)) {

        addAndMakeVisible(editor.get());
        addAndMakeVisible(closeButton);

        childBoundsChanged (editor.get());

        closeButton.onClick = std::forward<Callback> (onClose);
    }

    void setScaleFactor (float scale);
    void resized() override;
    void childBoundsChanged (juce::Component* child) override;

private:
    static constexpr auto buttonHeight = 40;

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::TextButton closeButton { "Close Plugin" };
};

//==============================================================================
class ScaledDocumentWindow final : public juce::DocumentWindow
{
public:
    ScaledDocumentWindow (juce::Colour bg, float scale);

    inline float getDesktopScaleFactor() const override { return juce::Desktop::getInstance().getGlobalScaleFactor() * desktopScale; }

private:
    float desktopScale = 1.0f;
};

//==============================================================================
class HostAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HostAudioProcessorEditor (HostAudioProcessorImpl& owner);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void childBoundsChanged (Component* child) override;
    void setScaleFactor (float scale) override;

private:
    void pluginChanged();
    void clearPlugin();

    static constexpr auto buttonHeight = 30;

    HostAudioProcessorImpl& hostProcessor;
    PluginLoaderComponent loader;
    std::unique_ptr<Component> editor;
    PluginEditorComponent* currentEditorComponent = nullptr;
    juce::ScopedValueSetter<std::function<void()>> scopedCallback;
    juce::TextButton closeButton { "Close Plugin" };
    float currentScaleFactor = 1.0f;
};

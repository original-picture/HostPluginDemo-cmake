#pragma once

#include "PluginProcessor.h"

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


        // this is a lambda that returns a lambda that calls a lambda lol
        // the outermost lambda is used to select the EditorStyle
        // the lambda one layer in gathers the information it needs to call callback, the innermost lambdas
        // --original-picture
        const auto getCallback = [this, &list, cb = std::forward<Callback> (callback)] (EditorStyle style)
        {
            return [this, &list, cb, style]
            {
                const auto index = pluginListComponent.getTableListBox().getSelectedRow();
                const auto& types = list.getTypes();

                if (juce::isPositiveAndBelow (index, types.size())) {
                    juce::NullCheckedInvocation::invoke (cb, types.getReference (index), style);
                }
            };
        };

        buttons.thisWindowButton.onClick = getCallback (EditorStyle::thisWindow);
        buttons.newWindowButton .onClick = getCallback (EditorStyle::newWindow);
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
    PluginEditorComponent (std::unique_ptr<juce::AudioProcessorEditor> editorIn, Callback&& onClose, EditorStyle style)
            : editor (std::move (editorIn)), editor_style_(style) {

        addAndMakeVisible(editor.get());

        if(style == EditorStyle::thisWindow) {
            addAndMakeVisible(closeButton); // if running in a new window, just use the native window close button
        }

        childBoundsChanged (editor.get());

        closeButton.onClick = std::forward<Callback> (onClose);
    }

    void setScaleFactor (float scale);
    void resized() override;
    void childBoundsChanged (juce::Component* child) override;

    ~PluginEditorComponent();

private:
    EditorStyle editor_style_;

    static constexpr auto buttonHeight = 40;

    std::unique_ptr<juce::AudioProcessorEditor> editor;
    juce::TextButton closeButton { "Close Plugin" };
};

//==============================================================================
class HostAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit HostAudioProcessorEditor (HostAudioProcessor& owner);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void childBoundsChanged (Component* child) override;
    void setScaleFactor (float scale) override;

    void pluginChanged();
    void clearPlugin();

private:
    void create_inner_plugin_editor_();
    ~HostAudioProcessorEditor() override;

    static constexpr auto buttonHeight = 30;

    HostAudioProcessor& hostProcessor;
    PluginLoaderComponent loader;
    std::unique_ptr<Component> editor;
    PluginEditorComponent* currentEditorComponent = nullptr;
    juce::ScopedValueSetter<std::function<void()>> scopedCallback; // a ScopedValueSetter is used here in order to automatically
    juce::TextButton closeButton { "Close Plugin" };               // reset the processor's pluginChanged callback to null if the editor gets destroyed
    float currentScaleFactor = 1.0f;                               // the processor then uses juce::NullCheckedInvocation::invoke()
};                                                                 // in order to avoid calling HostAudioProcessorEditor::pluginChanged() with a dangling this pointer --original-picture

//==============================================================================
class ScaledDocumentWindow final : public juce::DocumentWindow
{
public:
    ScaledDocumentWindow (juce::Colour bg, float scale, HostAudioProcessorEditor& owning_editor);

    inline float getDesktopScaleFactor() const override { return juce::Desktop::getInstance().getGlobalScaleFactor() * desktopScale; }

    virtual void closeButtonPressed();

    /** Callback that is triggered when the minimise button is pressed.

        This function is only called when using a non-native titlebar.

        The default implementation of this calls ResizableWindow::setMinimised(), but
        you can override it to do more customised behaviour.
    */
    virtual void minimiseButtonPressed();



private:
    HostAudioProcessorEditor& owning_editor_;
    float desktopScale = 1.0f;
};
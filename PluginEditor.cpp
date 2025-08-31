
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "native_window_system.h"
//
//==============================================================================


static void doLayout (juce::Component* main, juce::Component& bottom, int bottomHeight, juce::Rectangle<int> bounds) {
    juce::Grid grid;
    grid.setGap (juce::Grid::Px { margin });
    grid.templateColumns = { juce::Grid::TrackInfo { juce::Grid::Fr { 1 } } };
    grid.templateRows = {juce:: Grid::TrackInfo { juce::Grid::Fr { 1 } },
                         juce::Grid::TrackInfo { juce::Grid::Px { bottomHeight }} };
    grid.items = { juce::GridItem { main }, juce::GridItem { bottom }.withMargin ({ 0, margin, margin, margin }) };
    grid.performLayout (bounds);
}



void PluginLoaderComponent::resized() {
    doLayout (&pluginListComponent, buttons, 80, getLocalBounds());
}

PluginLoaderComponent::Buttons::Buttons() {
    label.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (label);
    addAndMakeVisible (thisWindowButton);
    addAndMakeVisible (newWindowButton);
}

void PluginLoaderComponent::Buttons::resized() {
    juce::Grid vertical;
    vertical.autoFlow = juce::Grid::AutoFlow::row;
    vertical.setGap (juce::Grid::Px { margin });
    vertical.autoRows = vertical.autoColumns = juce::Grid::TrackInfo { juce::Grid::Fr { 1 } };
    vertical.items.insertMultiple (0, juce::GridItem{}, 2);
    vertical.performLayout (getLocalBounds());
    
    label.setBounds (vertical.items[0].currentBounds.toNearestInt());

    juce::Grid grid;
    grid.autoFlow = juce::Grid::AutoFlow::column;
    grid.setGap (juce::Grid::Px { margin });
    grid.autoRows = grid.autoColumns = juce::Grid::TrackInfo { juce::Grid::Fr { 1 } };
    grid.items = { juce::GridItem { thisWindowButton },
                   juce::GridItem { newWindowButton } };
    
    grid.performLayout (vertical.items[1].currentBounds.toNearestInt());
}




void PluginEditorComponent::setScaleFactor (float scale) {
    if (editor != nullptr)
        editor->setScaleFactor (scale);
}

void PluginEditorComponent::resized() {
    if(editor_style_ == EditorStyle::newWindow) {
        setBounds(editor->getX(), editor->getY(), editor->getWidth(), editor->getHeight()); // I changed this bit in order to get rid of the gray margins in the editor window --original-picture
    }
    else {
        doLayout (editor.get(), closeButton, buttonHeight, getLocalBounds()); // how it was originally in HostPluginDemo.h // FIXME: a juce assert fails here when using the 'X' in the upper right corner to close the inner plugin when the inner plugin is running in the same window as the host plugin
    }
}

void PluginEditorComponent::childBoundsChanged (Component* child) {
    if (child != editor.get())
    return;
    
    const auto size = editor != nullptr ? editor->getLocalBounds()
                                        : juce::Rectangle<int>();
    
    setSize (size.getWidth(), margin + buttonHeight + size.getHeight());
}

PluginEditorComponent::~PluginEditorComponent() {

}


ScaledDocumentWindow::ScaledDocumentWindow(juce::Colour bg, float scale, HostAudioProcessorEditor& owning_editor) : DocumentWindow ("Editor", bg, 0), desktopScale (scale), owning_editor_(owning_editor) {}

void ScaledDocumentWindow::closeButtonPressed() {
    owning_editor_.clearPlugin();
}

void ScaledDocumentWindow::minimiseButtonPressed() {
    setMinimised(true);
}



HostAudioProcessorEditor::HostAudioProcessorEditor(HostAudioProcessor& owner)   : AudioProcessorEditor (owner),
                                                                                      hostProcessor (owner),
                                                                                      loader (owner.pluginFormatManager,
                                                                                              owner.pluginList,
                                                                                              [&owner] (const juce::PluginDescription& pd,
                                                                                                        EditorStyle editorStyle)
                                                                                              {
                                                                                                  owner.setNewPlugin (pd, editorStyle);
                                                                                              }),
                                                                                      scopedCallback (owner.pluginChanged, [this] { pluginChanged(); })
{
    //set_handler();

    setSize (500, 500);
    setResizable (false, false);
    addAndMakeVisible (closeButton);
    addAndMakeVisible (loader);


    //hostProcessor.pluginChanged();

    create_inner_plugin_editor_();

    closeButton.onClick = [this] { clearPlugin(); };

}

void HostAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker());
}

void HostAudioProcessorEditor::resized() {
    closeButton.setBounds (getLocalBounds().withSizeKeepingCentre (200, buttonHeight));
    loader.setBounds (getLocalBounds());
}

void HostAudioProcessorEditor::childBoundsChanged (Component* child) {
    if (child != inner_plugin_editor_component_or_top_level_window_.get())
    return;
    
    const auto size = inner_plugin_editor_component_or_top_level_window_ != nullptr ? inner_plugin_editor_component_or_top_level_window_->getLocalBounds()
                                        : juce::Rectangle<int>();
    
    setSize (size.getWidth(), size.getHeight());
}

void HostAudioProcessorEditor::setScaleFactor (float scale) {
    currentScaleFactor = scale;
    AudioProcessorEditor::setScaleFactor (scale);
    
    [[maybe_unused]] const auto posted = juce::MessageManager::callAsync ([ref = SafePointer<HostAudioProcessorEditor> (this), scale]
                                                                    {
                                                                        if (auto* r = ref.getComponent())
                                                                            if (auto* e = r->inner_plugin_editor_component_ref_)
                                                                                e->setScaleFactor (scale);
                                                                    });
    
    jassert (posted);
}

void HostAudioProcessorEditor::pluginChanged() {

   // this->currentEditorComponent = nullptr;
   hostProcessor.swap_read_write();
   create_inner_plugin_editor_();
}

void HostAudioProcessorEditor::clearPlugin() {
    inner_plugin_editor_component_ref_ = nullptr;
    inner_plugin_editor_component_or_top_level_window_ = nullptr;
    hostProcessor.clearPlugin();
}

void HostAudioProcessorEditor::create_inner_plugin_editor_() {
    //loader.setVisible (true);
    closeButton.setVisible(hostProcessor.processor_read_inner() != nullptr);

    if (hostProcessor.processor_read_inner() != nullptr) // I think part of the reason why this check is necessary because when we call createInnerEditor() on the next line,
                                                         // we'll be dereferencing one of the elements of inner_ping_pong
                                                         // it's basically a null check (I think) --original-picture        // just in case you aren't super familiar with unique_ptr,
    {                                                                                                                       // the lambda is the deleter --original-picture
        auto editorComponent = std::make_unique<PluginEditorComponent> (hostProcessor.createInnerEditor(), [this]
                                                                        {
                                                                            [[maybe_unused]] const auto posted = juce::MessageManager::callAsync ([this] { clearPlugin(); });
                                                                            jassert (posted);
                                                                        },
                                                                        hostProcessor.getEditorStyle());

        editorComponent->setScaleFactor (currentScaleFactor);
        inner_plugin_editor_component_ref_ = editorComponent.get(); // it's okay to store the raw pointer of this local unique_ptr (editorComponent) in a member variable (currentEditorComponent)
                                                                    // because editorComponent gets moved into editor (also a member variable) in the next piece of code, thereby extending its lifetime --original-picture

        switch (hostProcessor.getEditorStyle()) // this logic was originally inside a lambda that returned a Component that then got assigned to editor
        {                                       // I thought that was confusing, so I reimplemented it without the lambda --original-picture
            case EditorStyle::thisWindow:
            {
                addAndMakeVisible (editorComponent.get());
                setSize (editorComponent->getWidth(), editorComponent->getHeight());
                inner_plugin_editor_component_or_top_level_window_ = std::move (editorComponent);
                break;
            }

            case EditorStyle::newWindow:
            {
                const auto bg = getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId).darker();
                auto window = std::make_unique<ScaledDocumentWindow> (bg, currentScaleFactor, *this);
                window->setUsingNativeTitleBar(true);
                window->setName(hostProcessor.processor_read_inner()->getName());
                window->setTitleBarButtonsRequired(juce::DocumentWindow::minimiseButton|juce::DocumentWindow::closeButton, false);
                window->setContentOwned (editorComponent.release(), true);
                window->centreAroundComponent (this, window->getWidth(), window->getHeight());
                window->setVisible (true);

                //window->addToDesktop(0, this->getPeer()->getNativeHandle());
                this->getPeer()->addFloatingChildPeer(window->getPeer());
                //window->setTransientFor(this);

                closeButton.addToDesktop(0, this->getPeer()->getNativeHandle());
                //set_component_native_owning_window(*window, *this); // I added this --original-picture
                //window->setAlwaysOnTop (true);                    // this is how it was in the original juce version --original-picture
                inner_plugin_editor_component_or_top_level_window_ = std::move(window);
                break;
            }

            default:
                jassertfalse;
        }
    }
    else
    {
        inner_plugin_editor_component_or_top_level_window_ = nullptr;
        setSize (500, 500);
    }
}

HostAudioProcessorEditor::~HostAudioProcessorEditor() {

}

#include "PluginProcessor.h"
#include "PluginEditor.h"


HostAudioProcessorImpl::HostAudioProcessorImpl()
        : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                  .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    appProperties.setStorageParameters ([&]
                                        {
                                            juce::PropertiesFile::Options opt;
                                            opt.applicationName = getName();
                                            opt.commonToAllUsers = false;
                                            opt.doNotSave = false;
                                            opt.filenameSuffix = ".props";
                                            opt.ignoreCaseOfKeyNames = false;
                                            opt.storageFormat = juce::PropertiesFile::StorageFormat::storeAsXML;
                                            opt.osxLibrarySubFolder = "Application Support";
                                            return opt;
                                        }());

    pluginFormatManager.addDefaultFormats();

    if (auto savedPluginList = appProperties.getUserSettings()->getXmlValue ("pluginList"))
        pluginList.recreateFromXml (*savedPluginList);

    juce::MessageManagerLock lock;
    pluginList.addChangeListener (this);
}

bool HostAudioProcessorImpl::isBusesLayoutSupported (const BusesLayout& layouts) const  {
    buses_layout = layouts;

    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();
    
    if (! mainInput.isDisabled() && mainInput != mainOutput)
    return false;
    
    if (mainOutput.size() > 2)
    return false;
    
    return true;
}

void HostAudioProcessorImpl::prepareToPlay (double sr, int bs) {
    const juce::ScopedLock sl (innerMutex);

    active = true;

    if (inactive_inner() != nullptr) {
        inactive_inner()->setRateAndBufferSizeDetails (sr, bs);
        inactive_inner()->prepareToPlay (sr, bs);
    }
}

void HostAudioProcessorImpl::releaseResources() {
    const juce::ScopedLock sl (innerMutex);

    active = false;

    if (inactive_inner() != nullptr)
        inactive_inner()->releaseResources();

    if (active_inner() != nullptr)
        active_inner()->releaseResources();
}

void HostAudioProcessorImpl::reset() {
    const juce::ScopedLock sl (innerMutex);

    if (active_inner())
        active_inner()->reset();
}

// In this example, we don't actually pass any audio through the inner processor.
// In a 'real' plugin, we'd need to add some synchronisation to ensure that the inner
// plugin instance was never modified (deleted, replaced etc.) during a call to processBlock.
void HostAudioProcessorImpl::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_buffer) {
    jassert (! isUsingDoublePrecision());

    bool current_inner_index = active_ping_pong_index_;

    auto& inner = inner_ping_pong[current_inner_index];
    if(inner) {
        inner->processBlock(audio_buffer, midi_buffer);
    }

    /*for(unsigned channel_i = 0; channel_i < buf.getNumChannels(); ++channel_i) {
        auto dest = buf.getWritePointer(channel_i);
        auto src = buf.getReadPointer(channel_i);

        for(unsigned sample_i = 0; sample_i < buf.getNumSamples(); ++sample_i) {
            dest[sample_i] = src[sample_i];
        }
    }*/
}

void HostAudioProcessorImpl::processBlock (juce::AudioBuffer<double>& audio_buffer, juce::MidiBuffer& midi_buffer) {
    jassert (! isUsingDoublePrecision());

    bool current_inner_index = active_ping_pong_index_;

    auto& inner = inner_ping_pong[current_inner_index];
    if(inner) {
        inner->processBlock(audio_buffer, midi_buffer);
    }
}

void HostAudioProcessorImpl::getStateInformation (juce::MemoryBlock& destData) {
    const juce::ScopedLock sl (innerMutex);

    juce::XmlElement xml ("state");

    if(active_inner() != nullptr) {
        xml.setAttribute (editorStyleTag, (int) editorStyle);
        xml.addChildElement (active_inner()->getPluginDescription().createXml().release());
        xml.addChildElement (
            [this] {
                juce::MemoryBlock innerState;
                active_inner()->getStateInformation (innerState);

                auto stateNode = std::make_unique<juce::XmlElement> (innerStateTag);
                stateNode->addTextElement (innerState.toBase64Encoding());
                return stateNode.release();
            }());
    }

    const auto text = xml.toString();
    destData.replaceAll (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

void HostAudioProcessorImpl::setStateInformation (const void* data, int sizeInBytes) {
    const juce::ScopedLock sl (innerMutex);

    auto xml = juce::XmlDocument::parse (juce::String (juce::CharPointer_UTF8 (static_cast<const char*> (data)), (size_t) sizeInBytes));

    if(auto* pluginNode = xml->getChildByName ("PLUGIN")) {
        juce::PluginDescription pd;
        pd.loadFromXml (*pluginNode);

        juce::MemoryBlock innerState;
        innerState.fromBase64Encoding (xml->getChildElementAllSubText (innerStateTag, {}));

        setNewPlugin (pd,
        (EditorStyle) xml->getIntAttribute (editorStyleTag, 0),
        innerState);
    }
}

void HostAudioProcessorImpl::setNewPlugin(const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb) {
    const juce::ScopedLock sl (innerMutex);

    const auto callback = [this, where, mb] (std::unique_ptr<juce::AudioPluginInstance> instance, const juce::String& error)
    {
        if (error.isNotEmpty())
        {
            auto options = juce::MessageBoxOptions::makeOptionsOk (juce::MessageBoxIconType::WarningIcon,
                                                             "Plugin Load Failed",
                                                             error);
            messageBox = juce::AlertWindow::showScopedAsync (options, nullptr);
            return;
        }

        inactive_inner() = std::move (instance);
        editorStyle = where;

        if (inactive_inner() != nullptr && ! mb.isEmpty())
            inactive_inner()->setStateInformation (mb.getData(), (int) mb.getSize());

        // In a 'real' plugin, we'd also need to set the bus configuration of the inner plugin.
        // One possibility would be to match the bus configuration of the wrapper plugin, but
        // the inner plugin isn't guaranteed to support the same layout. Alternatively, we
        // could try to apply a reasonably similar layout, and maintain a mapping between the
        // inner/outer channel layouts.
        //
        // In any case, it is essential that the inner plugin is told about the bus
        // configuration that will be used. The AudioBuffer passed to the inner plugin must also
        // exactly match this layout.

        if(active) {
            if(!inactive_inner()->checkBusesLayoutSupported(buses_layout)) {
                // this is called from the gui thread so it's okay
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Error!",
                                                                                                           "the plugin you're trying to load doesn't support the bus layout of the host plugin!",
                                                                                                           "okay ;_;");

                inactive_inner().reset();
            }
            else {
                jassert(inactive_inner()->setBusesLayout(buses_layout));
                inactive_inner()->setRateAndBufferSizeDetails (getSampleRate(), getBlockSize());
                inactive_inner()->prepareToPlay (getSampleRate(), getBlockSize());

                juce::NullCheckedInvocation::invoke (pluginChanged);
            }
        }

        // juce::NullCheckedInvocation::invoke (pluginChanged);
    };

    pluginFormatManager.createPluginInstanceAsync (pd, getSampleRate(), getBlockSize(), callback);
}

void HostAudioProcessorImpl::clearPlugin() {
    const juce::ScopedLock sl (innerMutex);

    inactive_inner() = nullptr;
    juce::NullCheckedInvocation::invoke (pluginChanged);
}

bool HostAudioProcessorImpl::isPluginLoaded() const {
    const juce::ScopedLock sl (innerMutex);
    return active_inner() != nullptr;
}

std::unique_ptr<juce::AudioProcessorEditor> HostAudioProcessorImpl::createInnerEditor() const {
    const juce::ScopedLock sl (innerMutex);
    return rawToUniquePtr (active_inner()->hasEditor() ? active_inner()->createEditorIfNeeded() : nullptr);
}

void HostAudioProcessorImpl::changeListenerCallback (juce::ChangeBroadcaster* source) {
    if(source != &pluginList) {
        return;
    }

    if (auto savedPluginList = pluginList.createXml()) {
        appProperties.getUserSettings()->setValue ("pluginList", savedPluginList.get());
        appProperties.saveIfNeeded();
    }
}

void HostAudioProcessorImpl::swap_active_inactive() {
                                              // xoring with 1 is equivalent to boolean negation
    active_ping_pong_index_.fetch_xor(1, std::memory_order_release); // use release because we need to make sure that all of our pointers have been set by the time the audio thread sees the new value of this variable

    //inactive_inner().reset();
}





juce::AudioProcessorEditor* HostAudioProcessor::createEditor() { return new HostAudioProcessorEditor (*this); }


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HostAudioProcessor();
}


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
    
    if (inner != nullptr) {
        inner->setRateAndBufferSizeDetails (sr, bs);
        inner->prepareToPlay (sr, bs);
    }
}

void HostAudioProcessorImpl::releaseResources() {
    const juce::ScopedLock sl (innerMutex);
    
    active = false;
    
    if (inner != nullptr)
        inner->releaseResources();
}

void HostAudioProcessorImpl::reset() {
    const juce::ScopedLock sl (innerMutex);
    
    if (inner != nullptr)
    inner->reset();
}

// In this example, we don't actually pass any audio through the inner processor.
// In a 'real' plugin, we'd need to add some synchronisation to ensure that the inner
// plugin instance was never modified (deleted, replaced etc.) during a call to processBlock.
void HostAudioProcessorImpl::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&) {
    jassert (! isUsingDoublePrecision());

    for(unsigned channel_i = 0; channel_i < buf.getNumChannels(); ++channel_i) {
        auto dest = buf.getWritePointer(channel_i);
        auto src = buf.getReadPointer(channel_i);

        for(unsigned sample_i = 0; sample_i < buf.getNumSamples(); ++sample_i) {
            dest[sample_i] = src[sample_i];
        }
    }
}

void HostAudioProcessorImpl::processBlock (juce::AudioBuffer<double>& buf, juce::MidiBuffer&) {
    jassert (isUsingDoublePrecision());

    for(unsigned channel_i = 0; channel_i < buf.getNumChannels(); ++channel_i) {
        auto dest = buf.getWritePointer(channel_i);
        auto src = buf.getReadPointer(channel_i);

        for(unsigned sample_i = 0; sample_i < buf.getNumSamples(); ++sample_i) {
            dest[sample_i] = src[sample_i];
        }
    }
}

void HostAudioProcessorImpl::getStateInformation (juce::MemoryBlock& destData) {
    const juce::ScopedLock sl (innerMutex);

    juce::XmlElement xml ("state");

    if(inner != nullptr) {
        xml.setAttribute (editorStyleTag, (int) editorStyle);
        xml.addChildElement (inner->getPluginDescription().createXml().release());
        xml.addChildElement (
            [this] {
                juce::MemoryBlock innerState;
                inner->getStateInformation (innerState);

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

        inner = std::move (instance);
        editorStyle = where;

        if (inner != nullptr && ! mb.isEmpty())
            inner->setStateInformation (mb.getData(), (int) mb.getSize());

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
            inner->setRateAndBufferSizeDetails (getSampleRate(), getBlockSize());
            inner->prepareToPlay (getSampleRate(), getBlockSize());
        }

        juce::NullCheckedInvocation::invoke (pluginChanged);
    };

    pluginFormatManager.createPluginInstanceAsync (pd, getSampleRate(), getBlockSize(), callback);
}

void HostAudioProcessorImpl::clearPlugin() {
    const juce::ScopedLock sl (innerMutex);

    inner = nullptr;
    juce::NullCheckedInvocation::invoke (pluginChanged);
}

bool HostAudioProcessorImpl::isPluginLoaded() const {
    const juce::ScopedLock sl (innerMutex);
    return inner != nullptr;
}

std::unique_ptr<juce::AudioProcessorEditor> HostAudioProcessorImpl::createInnerEditor() const {
    const juce::ScopedLock sl (innerMutex);
    return rawToUniquePtr (inner->hasEditor() ? inner->createEditorIfNeeded() : nullptr);
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



juce::AudioProcessorEditor* HostAudioProcessor::createEditor() { return new HostAudioProcessorEditor (*this); }


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HostAudioProcessor();
}

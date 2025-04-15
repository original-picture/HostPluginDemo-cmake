#include "PluginProcessor.h"
#include "PluginEditor.h"


HostAudioProcessor::HostAudioProcessor()
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

    for(unsigned i = 0; i < maximum_number_of_parameters_; ++i) {
        parameters_.emplace_back(new forwarding_parameter_ptr(i));
        addParameter(parameters_[i]);
    }
}

bool HostAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const  {
    const auto& mainOutput = layouts.getMainOutputChannelSet();
    const auto& mainInput  = layouts.getMainInputChannelSet();
    
    if (! mainInput.isDisabled() && mainInput != mainOutput)
    return false;
    
    if (mainOutput.size() > 2)
    return false;
    
    return true;
}

void HostAudioProcessor::prepareToPlay (double sr, int bs) {
    const juce::ScopedLock sl (innerMutex);

    active = true;

    if (editor_write_inner() != nullptr) { // TODO: do I need to do processor_read_inner() here too? Also shouldn't I be setting the bus layout too?
        editor_write_inner()->setRateAndBufferSizeDetails (sr, bs);
        editor_write_inner()->prepareToPlay (sr, bs);
    }
}

void HostAudioProcessor::releaseResources() {
    const juce::ScopedLock sl (innerMutex);

    active = false;

    if (editor_write_inner() != nullptr)
        editor_write_inner()->releaseResources();

    if (processor_read_inner() != nullptr)
        processor_read_inner()->releaseResources();
}

void HostAudioProcessor::reset() {
    const juce::ScopedLock sl (innerMutex);

    if (processor_read_inner())
        processor_read_inner()->reset();
}

// In this example, we don't actually pass any audio through the inner processor.
// In a 'real' plugin, we'd need to add some synchronisation to ensure that the inner
// plugin instance was never modified (deleted, replaced etc.) during a call to processBlock.
void HostAudioProcessor::processBlock (juce::AudioBuffer<float>& audio_buffer, juce::MidiBuffer& midi_buffer) {
    jassert (! isUsingDoublePrecision());

    bool current_inner_index = processor_read_ping_pong_index_;

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

void HostAudioProcessor::processBlock (juce::AudioBuffer<double>& audio_buffer, juce::MidiBuffer& midi_buffer) {
    jassert (! isUsingDoublePrecision());

    bool current_inner_index = processor_read_ping_pong_index_;

    auto& inner = inner_ping_pong[current_inner_index];
    if(inner) {
        inner->processBlock(audio_buffer, midi_buffer);
    }
}

void HostAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {
    const juce::ScopedLock sl (innerMutex); // FIXME there's a data race here because this usually gets called from the message thread, but I'm accessing processor_read_inner(), which belongs to the audio thread
                                            // --original-picture
    juce::XmlElement xml ("state");

    if(processor_read_inner() != nullptr) {
        xml.setAttribute (editorStyleTag, (int) editorStyle);
        xml.addChildElement (processor_read_inner()->getPluginDescription().createXml().release());
        xml.addChildElement (
            [this] {
                juce::MemoryBlock innerState;
                processor_read_inner()->getStateInformation (innerState); // TODO: could just temporarily swap them so that i can work on processor_read_inner()
                                                                  // aaaggh but no that wouldn't work because processBlock could still be working on it   --original-picture
                auto stateNode = std::make_unique<juce::XmlElement> (innerStateTag);
                stateNode->addTextElement (innerState.toBase64Encoding());
                return stateNode.release();
            }());
    }

    const auto text = xml.toString();
    destData.replaceAll (text.toRawUTF8(), text.getNumBytesAsUTF8());
}

void HostAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {
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

void HostAudioProcessor::setNewPlugin(const juce::PluginDescription& pd, EditorStyle where, const juce::MemoryBlock& mb) {
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

        editor_write_inner() = std::move (instance);
        editorStyle = where;

        if (editor_write_inner() != nullptr && ! mb.isEmpty())
            editor_write_inner()->setStateInformation (mb.getData(), (int) mb.getSize());

        // In a 'real' plugin, we'd also need to set the bus configuration of the inner plugin.
        // One possibility would be to match the bus configuration of the wrapper plugin, but
        // the inner plugin isn't guaranteed to support the same layout. Alternatively, we
        // could try to apply a reasonably similar layout, and maintain a mapping between the
        // inner/outer channel layouts.
        //
        // In any case, it is essential that the inner plugin is told about the bus
        // configuration that will be used. The AudioBuffer passed to the inner plugin must also
        // exactly match this layout.

        // ^
        // I did what the juce people described here
        // --original-picture


        if(active) { // I don't understand what active does --original-picture
            if(!editor_write_inner()->checkBusesLayoutSupported(getBusesLayout())) {
                // this is called from the gui thread so it's okay --original-picture
                juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                       "Error!",
                                                       "the plugin you're trying to load doesn't support the bus layout of the host plugin!",
                                                       "okay ;_;");

                editor_write_inner().reset();
            }
            else {
                jassert(editor_write_inner()->setBusesLayout(getBusesLayout()));
                editor_write_inner()->setRateAndBufferSizeDetails (getSampleRate(), getBlockSize());
                editor_write_inner()->prepareToPlay (getSampleRate(), getBlockSize());

                for(unsigned outer_parameter_i = 0; outer_parameter_i < maximum_number_of_parameters_; ++outer_parameter_i) {
                    parameters_[outer_parameter_i]->set_forwarded_parameter(nullptr);
                }

                unsigned inner_plugin_parameter_count = editor_write_inner()->getParameters().size(),
                         number_of_parameters;

                if(editor_write_inner()->getParameters().size() > maximum_number_of_parameters_) {
                    number_of_parameters = maximum_number_of_parameters_;
                    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                                           "Warning!",
                                                           "The plugin you're trying to load has more parameters than the hardcoded maximum of the host plugin (" + juce::String(inner_plugin_parameter_count) + " vs " + juce::String(maximum_number_of_parameters_) + ")! The plugin can still be used, but the last " + juce::String(inner_plugin_parameter_count - maximum_number_of_parameters_) + " will be inaccessible from your DAW!",
                                                           "okay ;_;");
                }
                else {
                    number_of_parameters = inner_plugin_parameter_count;
                }

                for(unsigned inner_parameter_i = 0; inner_parameter_i < number_of_parameters; ++inner_parameter_i) {
                    parameters_[inner_parameter_i]->set_forwarded_parameter(editor_write_inner()->getParameters()[inner_parameter_i]);
                }

                updateHostDisplay();

                juce::NullCheckedInvocation::invoke (pluginChanged); // I moved this up here and I can't remember why lol
            }                                                           // I've tested both versions and they seem to do the same thing,
        }                                                               // but that could be because any difference in behavior is masked by the double-load bug --original-picture

        // juce::NullCheckedInvocation::invoke (pluginChanged); // this line is how it is in the original HostPluginDemo.h --original-picture
    };

    pluginFormatManager.createPluginInstanceAsync (pd, getSampleRate(), getBlockSize(), callback);
}

void HostAudioProcessor::clearPlugin() {
    const juce::ScopedLock sl (innerMutex);

    for(unsigned outer_parameter_i = 0; outer_parameter_i < maximum_number_of_parameters_; ++outer_parameter_i) {
        parameters_[outer_parameter_i]->set_forwarded_parameter(nullptr);
    }
    updateHostDisplay();

    editor_write_inner() = nullptr; // TODO: shouldn't this be processor_read_inner?
    juce::NullCheckedInvocation::invoke (pluginChanged);
}

bool HostAudioProcessor::isPluginLoaded() const {
    const juce::ScopedLock sl (innerMutex);
    return processor_read_inner() != nullptr;
}

std::unique_ptr<juce::AudioProcessorEditor> HostAudioProcessor::createInnerEditor() const {
    const juce::ScopedLock sl (innerMutex);
    return rawToUniquePtr (editor_write_inner()->hasEditor() ? editor_write_inner()->createEditorIfNeeded() : nullptr);
}

void HostAudioProcessor::changeListenerCallback (juce::ChangeBroadcaster* source) {
    if(source != &pluginList) {
        return;
    }

    if (auto savedPluginList = pluginList.createXml()) {
        appProperties.getUserSettings()->setValue ("pluginList", savedPluginList.get());
        appProperties.saveIfNeeded();
    }
}

void HostAudioProcessor::swap_read_write() {
                                              // xoring with 1 is equivalent to boolean negation
    processor_read_ping_pong_index_.fetch_xor(1, std::memory_order_release); // use release because we need to make sure that all of our pointers have been set by the time the audio thread sees the new value of this variable
}



juce::AudioProcessorEditor* HostAudioProcessor::createEditor() { return new HostAudioProcessorEditor (*this); }


juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HostAudioProcessor();
}


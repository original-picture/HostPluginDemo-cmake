
#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

/**
 *  this is a simple helper class that is used to forward parameters from the inner plugin to the outer plugin
 *  It holds a pointer to a HostedAudioProcessorParameter and forwards all its virtual member functions inherited from juce::AudioProcessorParameter
 *  to this pointer
 *  an object of this type can be empty (the internal pointer is null). In this case, all of the virtual member functions will return default values
 *
 *  thanks to eyalamir from the juce forums for giving me this idea https://forum.juce.com/t/forwarding-the-parameters-of-a-hosted-plugin/65751/2?u=original-picture
 */
class forwarding_parameter_ptr : public juce::AudioProcessorParameter {
    juce::HostedAudioProcessorParameter* forwarded_parameter_ = nullptr;
    juce::String placeholder_name_;

public:
    /// placeholder_name is a string to use if this object points to a parameter (is not null)
    forwarding_parameter_ptr(const juce::String& placeholder_name);

    /// does the same thing as create_with_placeholder_name_from_index
    forwarding_parameter_ptr(unsigned parameter_index);

    static forwarding_parameter_ptr create_with_placeholder_name_from_index(unsigned parameter_index);

    void set_forwarded_parameter(juce::HostedAudioProcessorParameter* parameter_to_forward);

    /// returns true if this object points to a parameter (is not null)
    operator bool() const;

    /// all these member functions just forward the corresponding member functions of the parameter that this object points to
    /// if there is no forwarded parameter, they return default values
    virtual float getValue() const;
    virtual void setValue (float newValue);
    virtual float getDefaultValue() const;
    virtual juce::String getName (int maximumStringLength) const;
    virtual juce::String getLabel() const;
    virtual int getNumSteps() const;
    virtual bool isDiscrete() const;
    virtual bool isBoolean() const;
    virtual juce::String getText (float normalisedValue, int /*maximumStringLength*/) const;
    virtual float getValueForText (const juce::String& text) const;
    virtual bool isOrientationInverted() const;
    virtual bool isAutomatable() const;
    virtual bool isMetaParameter() const;
    virtual Category getCategory() const;
    //virtual juce::String getCurrentValueAsText() const;
    virtual juce::StringArray getAllValueStrings() const;

    virtual ~forwarding_parameter_ptr() = default;
};
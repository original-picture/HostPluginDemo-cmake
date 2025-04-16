
#pragma once

#include "juce_audio_processors/juce_audio_processors.h"

/**
 * this file and forwarding_parameter_ptr.cpp were written by me (original-picture), not the juce people
 *
 * this is a simple helper class that is used to forward parameters from the inner plugin to the outer plugin
 * It holds a pointer to a AudioProcessorParameter and forwards all its virtual member functions inherited from juce::AudioProcessorParameter
 * to this pointer
 * an object of this type can be empty (the internal pointer can be null). In this case, all of the virtual member functions will return default values
 *
 * thanks to eyalamir from the juce forums for giving me this idea https://forum.juce.com/t/forwarding-the-parameters-of-a-hosted-plugin/65751/2?u=original-picture
 */
class forwarding_parameter_ptr : public juce::AudioProcessorParameter,
                                 public juce::AudioProcessorParameter::Listener { // so we can get notified and call setValueNotifyingHost when the wrapped parameter changes
    juce::AudioProcessorParameter* forwarded_parameter_ = nullptr; // I was using HostedAudioProcessorParameter here, but that didn't work with the AudioPluginInstance in the processor,
    juce::String placeholder_name_;                                // because the getParameters member function of AudioPluginInstance return an array of AudioProcessorParameter, not HostedAudioProcessorParameter
                                                                   // idk maybe this is a sign that I should be doing something differently

    void parameterValueChanged  (int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override;

public:
    /// placeholder_name is a string to use if this object points to a parameter (is not null)
    forwarding_parameter_ptr(const juce::String& placeholder_name);

    /// does the same thing as create_with_placeholder_name_from_index
    forwarding_parameter_ptr(unsigned parameter_index);

    static forwarding_parameter_ptr create_with_placeholder_name_from_index(unsigned parameter_index);

    void set_forwarded_parameter(juce::AudioProcessorParameter* parameter_to_forward);


    // delete all copy/move constructors/assignment operators because everything breaks if the address of an object changes (because this pointer is registered as a listener)
    // doesn't really change anything in practice, because juce parameters are always dynamically allocated individually
    // just prevents a user from doing something dangerous
    forwarding_parameter_ptr(const forwarding_parameter_ptr&) = delete;
    forwarding_parameter_ptr(forwarding_parameter_ptr&&) = delete;

    forwarding_parameter_ptr& operator=(const forwarding_parameter_ptr&) = delete;
    forwarding_parameter_ptr& operator=(forwarding_parameter_ptr&&) = delete;

    /// returns true if this object points to a parameter (is not null)
    operator bool() const;

    /// every member function here just forwards the corresponding member function of the parameter that this object points to
    /// if there is no forwarded parameter, they return default values
    virtual float getValue() const;
    virtual void setValue (float newValue);
    virtual float getDefaultValue() const;
    virtual juce::String getName (int maximumStringLength) const;
    virtual juce::String getLabel() const;
    virtual int getNumSteps() const;
    virtual bool isDiscrete() const;
    virtual bool isBoolean() const;
    virtual juce::String getText (float normalisedValue, int maximumStringLength) const;
    virtual float getValueForText (const juce::String& text) const;
    virtual bool isOrientationInverted() const;
    virtual bool isAutomatable() const;
    virtual bool isMetaParameter() const;
    virtual Category getCategory() const;
    //virtual juce::String getCurrentValueAsText() const;
    virtual juce::StringArray getAllValueStrings() const;

    virtual ~forwarding_parameter_ptr() = default;
};
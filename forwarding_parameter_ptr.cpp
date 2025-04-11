
#include "forwarding_parameter_ptr.h"

forwarding_parameter_ptr::forwarding_parameter_ptr(const juce::String& placeholder_name) : placeholder_name_(placeholder_name) {}

forwarding_parameter_ptr::forwarding_parameter_ptr(unsigned int parameter_index) : placeholder_name_("Unused parameter " + juce::String(parameter_index)) {}

forwarding_parameter_ptr forwarding_parameter_ptr::create_with_placeholder_name_from_index(unsigned int parameter_index) {
    return {parameter_index};
}

forwarding_parameter_ptr::operator bool() const {
    return forwarded_parameter_;
}


float forwarding_parameter_ptr::getValue() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getValue();
    }
    else {
        return 0.f;
    }
}

void forwarding_parameter_ptr::set_forwarded_parameter(juce::HostedAudioProcessorParameter* parameter_to_forward) {
    forwarded_parameter_ = parameter_to_forward;
}

void forwarding_parameter_ptr::setValue(float newValue) {
    if(forwarded_parameter_) {
        forwarded_parameter_->setValue(newValue);
    }
}

float forwarding_parameter_ptr::getDefaultValue() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getDefaultValue();
    }
    else {
        return 0.f;
    }
}

juce::String forwarding_parameter_ptr::getName (int maximumStringLength) const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getName(maximumStringLength);
    }
    else {
        return placeholder_name_;
    }
}

juce::String forwarding_parameter_ptr::getLabel() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getLabel();
    }
    else {
        return {};
    }
}

int forwarding_parameter_ptr::getNumSteps() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getNumSteps();
    }
    else {
        return 1;
    }
}

bool forwarding_parameter_ptr::isDiscrete() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->isDiscrete();
    }
    else {
        return true;
    }
}

bool forwarding_parameter_ptr::isBoolean() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->isDiscrete();
    }
    else {
        return false;
    }
}

juce::String forwarding_parameter_ptr::getText (float normalisedValue, int maximumStringLength) const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getText(normalisedValue, maximumStringLength);
    }
    else {
        return "No value -- parameter not in use";
    }
}

float forwarding_parameter_ptr::getValueForText (const juce::String& text) const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getValueForText(text);
    }
    else {
        return 0.f;
    }
}

bool forwarding_parameter_ptr::isOrientationInverted() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->isOrientationInverted();
    }
    else {
        return false;
    }
}

bool forwarding_parameter_ptr::isAutomatable() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->isAutomatable();
    }
    else {
        return true;
    }
}

bool forwarding_parameter_ptr::isMetaParameter() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->isMetaParameter();
    }
    else {
        return false;
    }
}

juce::AudioProcessorParameter::Category forwarding_parameter_ptr::getCategory() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getCategory();
    }
    else {
        return genericParameter;
    }
}

/*
juce::String forwarding_parameter_ptr::getCurrentValueAsText() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getCurrentValueAsText();
    }
    else {
        return getTe;
    }
}
*/
juce::StringArray forwarding_parameter_ptr::getAllValueStrings() const {
    if(forwarded_parameter_) {
        return forwarded_parameter_->getAllValueStrings();
    }
    else {
        return {"No values -- parameter not in use"};
    }
}
